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
#include <tuple>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <zmq.hpp>
#include "message.hpp"
#include "socket.hpp"
#include "helpers.hpp"


/*! \file broker.hpp
 * The message broker which routes and distributes work.
 */



/*! \brief Message broker, which routes and distributes work.
 */
class broker
    : public boost::static_visitor<>
{
    struct worker
    {
        msg::address address;
        std::string service;
        detail_time::time last_seen;
    };

    std::string const addr;
    auto const static socket_type = zmq::socket_type::router;
    std::chrono::milliseconds const worker_timeout;
    int run_max_wait_ms = 200;
    class socket sock;
    std::queue<msg::part_source> send_queue;

    std::unordered_map<msg::address, worker> workers;
    std::unordered_map<std::string, std::unordered_set<msg::address>> free_workers;
    std::unordered_map<std::string, std::queue<msg::request>> pending_requests;

    auto free_worker(worker const & worker) -> void;
    auto get_worker(decltype(free_workers[""]) & available_workers)
        -> boost::optional<worker &>;
public:
    /*! \brief Process a registration message. */
    auto operator()(msg::registration & msg) -> void;
    /*! \brief Process a heartbeat message. */
    auto operator()(msg::ping         & msg) -> void;
    /*! \brief Process a heartbeat response. */
    auto operator()(msg::pong         & msg) -> void;
    /*! \brief Process a work request. */
    auto operator()(msg::request      & msg) -> void;
    /*! \brief Process a work reply. */
    auto operator()(msg::reply        & msg) -> void;
    /*! \brief Process a reconnect message. */
    auto operator()(msg::reconnect    & msg) -> void;

    broker(zmq::context_t & ctx,
           std::string const & addr,
           std::chrono::milliseconds worker_timeout);

    /*! \brief Run the message broker for one iteration.
     *
     * This function should be called repeatedly to run the
     * broker. When called, if there are no messages to be recieved,
     * it will wait for some time.
     *
     * Instead of calling this function directly, consider using
     * [component](\ref component) to run the broker.
     */
    auto run() -> void;
};
