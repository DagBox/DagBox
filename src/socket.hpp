#pragma once

#include <vector>
#include <zmq.hpp>


/*! \file socket.hpp
 * Extensions on ZeroMQ's socket.
 *
 * We extend socket to be able to easily send and recieve multi-part
 * messages.
 */



/*! A ZeroMQ socket that can recieve messages containing multiple
 *  parts.
 */
class socket : public zmq::socket_t
{
public:
    // Just use regular socket's constructors
    using socket_t::socket_t;

    /*! \brief Recieve a message that has multiple parts as a stream.
     */
    auto recv_multimsg() -> std::vector<zmq::message_t>;

    /*! \brief Send a message that has multiple parts.
     */
    template <class iterator>
    auto send_multimsg(iterator first, iterator last) -> void
    {
        while (first != last) {
            auto part = std::move(*first);
            ++first;
            if (first != last) { // More parts to send
                send(part, ZMQ_SNDMORE);
            } else {
                send(part);
            }
        }
    }

};
