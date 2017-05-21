/*
  Copyright 2017 Kaan Genç

  This file is part of DagBox.

  DagBox is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  DagBox is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with DagBox.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <string>
#include <boost/variant.hpp>
#include <zmq.hpp>
#include <spdlog/spdlog.h>
#include "message.hpp"
#include "socket.hpp"
#include "helpers.hpp"

/*! \file assistant.hpp
 * Assistant class for running workers.
 */


/*! \brief A multi-part message that is ready to be sent over a socket. */
typedef std::vector<zmq::message_t> sendable;
/*! \brief A sendable message that may or may not exist. */
typedef boost::optional<sendable> maybe_sendable;


/*! \brief An assistant for running workers.
 *
 * Creates a socket, and handles sending pings to avoid timeouts.
 *
 * The `worker` class must have a public constructor, a member
 * `std::string const service_name` and a method `operator()(msg::request && request) -> std::vector<zmq::message_t>`.
 * See [datastore](\ref data::datastore) for an example.
 */
template <class worker>
class assistant
    : public boost::static_visitor<maybe_sendable>
{
    auto const static socket_type = zmq::socket_type::dealer;

    auto register_worker() -> sendable
    {
        return msg::send(msg::registration::make(work.service_name));
    }

    worker work;
    class socket sock;
    std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_st(work.service_name + " assistant");
public:
    /*! \brief Create an assistant that runs the `worker`.
     *
     * \param ctx 0MQ context the assistant will run in.
     * \param broker_addr The address of the broker the assistant
     * should connect to.
     * \param worker_timeout Time in miliseconds the broker will
     * consider a worker dead if there hasn't been any communication.
     * \param args The arguments to be passed to the constructor of
     * the worker.
     */
    template <class ... Args>
    assistant(zmq::context_t & ctx,
              std::string const & broker_addr,
              int worker_timeout,
              Args ... args)
        : work(args...),
          sock(ctx, socket_type),
          logger(spdlog::stdout_color_st(work.service_name + " assistant"))
    {
        // Timeout on recv so we can stop waiting and send a ping to the
        // broker
        sock.setsockopt(ZMQ_RCVTIMEO, worker_timeout);
        sock.connect(broker_addr);

        sock.send_multimsg(register_worker());
    }

    /*! \brief Run the worker for one iteration.
     *
     * The assistant will first try to accept a message from the
     * broker. If the message is a request then it will be passed to
     * the worker, otherwise it will be handled by the assistant.
     * Depending on the message, the assistant may or may not send a
     * message in response.
     */
    auto run() -> void {
        auto received = sock.recv_multimsg();
        if (received.size() == 0) {
            // Check if the broker is still alive
            sock.send_multimsg(msg::send(msg::ping::make()));
        } else {
            // If the recv didn't time out
            auto message = msg::read(std::move(received));
            auto maybe_reply = boost::apply_visitor(*this, message);
            if (maybe_reply) {
                sock.send_multimsg(std::move(*maybe_reply));
            }
        }
    }

    /*! \brief Process a registration message. */
    auto operator()(msg::registration & msg) -> maybe_sendable {
        logger->debug("Successfully registered for service {}", msg.service());
        return boost::none;
    }

    /*! \brief Process a heartbeat message. */
    auto operator()(msg::ping & msg) -> maybe_sendable {
        return msg::send(msg::pong::make(std::move(msg)));
    }

    /*! \brief Process a heartbeat response. */
    auto operator()(msg::pong &) -> maybe_sendable {
        return boost::none;
    }

    /*! \brief Process a work request. */
    auto operator()(msg::request & msg) -> maybe_sendable {
        return work(std::move(msg));
    }

    /*! \brief Process a work reply. */
    auto operator()(msg::reply &) -> maybe_sendable {
        logger->warn("Recieved unexpected reply");
        return boost::none;
    }
    /*! \brief Process a reconnect message. */
    auto operator()(msg::reconnect &) -> maybe_sendable {
        return register_worker();
    }
};
