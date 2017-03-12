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


namespace detail
{
    struct worker
    {
        msg::address address;
        std::string service;
        time last_seen;
    };
}



/*! \brief Message broker, which routes and distributes work.
 */
class broker
    : public boost::static_visitor<>
{
    auto const static socket_type = zmq::socket_type::router;
    socket sock;
    std::queue<msg::part_source> send_queue;
    broker(zmq::context_t & ctx, std::string const & addr);

    std::unordered_map<msg::address, detail::worker> workers;
    std::unordered_map<std::string, std::unordered_set<msg::address>> free_workers;
    std::unordered_map<std::string, std::queue<msg::request>> pending_requests;

    auto free_worker(detail::worker const & worker) -> void;
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

    /*! \brief Create and run a message broker.
     *
     * When this function is called, it will enter an **infinite
     * loop**. Do not call this function directly, create a new thread
     * or a process to run it.
     */
    auto static run(zmq::context_t & ctx, std::string const & addr) -> void;
};
