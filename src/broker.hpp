#pragma once

#include <string>
#include <tuple>
#include <boost/variant.hpp>
#include <zmq.hpp>
#include "message.hpp"
#include "socket.hpp"


/*! \file broker.hpp
 * The message broker which routes and distributes work.
 */



/*! \brief Message broker, which routes and distributes work.
 */
class broker
    : public boost::static_visitor<std::tuple<msg::message_iterator, msg::message_iterator>>
{
    auto const static socket_type = zmq::socket_type::router;
    socket sock;
    broker(zmq::context_t & ctx, std::string const & addr);
public:
    /*! \brief Process a registration message. */
    auto operator()(msg::registration & msg) -> std::tuple<msg::message_iterator, msg::message_iterator>;
    /*! \brief Process a heartbeat message. */
    auto operator()(msg::ping         & msg) -> std::tuple<msg::message_iterator, msg::message_iterator>;
    /*! \brief Process a heartbeat response. */
    auto operator()(msg::pong         & msg) -> std::tuple<msg::message_iterator, msg::message_iterator>;
    /*! \brief Process a work request. */
    auto operator()(msg::request      & msg) -> std::tuple<msg::message_iterator, msg::message_iterator>;
    /*! \brief Process a work reply. */
    auto operator()(msg::reply        & msg) -> std::tuple<msg::message_iterator, msg::message_iterator>;

    /*! \brief Create and run a message broker.
     *
     * When this function is called, it will enter an **infinite
     * loop**. Do not call this function directly, create a new thread
     * or a process to run it.
     */
    auto static run(zmq::context_t & ctx, std::string const & addr) -> void;
};
