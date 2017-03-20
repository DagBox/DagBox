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
#include "message.hpp"
#include "socket.hpp"
#include "helpers.hpp"


/*! \brief An assistant for running workers.
 *
 * Creates a socket, and handles sending pings to avoid timeouts.
 *
 * The `worker` class should be a visitor that can visit the message
 * classes, see [broker](\ref broker) for an example, or for the documentation
 * [boost::variant](http://www.boost.org/doc/libs/1_63_0/doc/html/variant.html).
 * The worker should return a `boost::optional<std::vector<zmq::message_t>>`.
 */
template <class worker>
class assistant
{
    auto const static socket_type = zmq::socket_type::dealer;

    worker work;
    socket sock;
public:
    /*! \brief Create an assistant that runs `worker`.
     *
     * \param args The arguments to be passed to the constructor of
     * the worker.
     */
    template <class ... Args>
    assistant(zmq::context_t & ctx,
              std::string const & broker_addr,
              int worker_timeout,
              Args ... args)
        : work(args...), sock(ctx, socket_type)
    {
        // Timeout on recv so we can stop waiting and send a ping to the
        // broker
        sock.setsockopt(ZMQ_RCVTIMEO, worker_timeout);
        sock.connect(broker_addr);
    }

    auto run() -> void {
        auto received = sock.recv_multimsg();
        if (received.size() == 0) {
            // Check if the broker is still alive
            sock.send_multimsg(msg::send(msg::ping::make()));
        } else {
            // If the recv didn't time out
            auto message = msg::read(std::move(received));
            auto maybe_reply = boost::apply_visitor(work, message);
            if (maybe_reply) {
                sock.send_multimsg(std::move(*maybe_reply));
            }
        }
    }
};
