#pragma omp once

#include <zmq.hpp>
#include <boost/coroutine2/coroutine.hpp>


/*! \file socket.hpp
 * Extensions on ZeroMQ's socket.
 *
 * We extend socket to be able to easily send and recieve multi-part
 * messages by creating a stream of message parts.
 */



/*! \brief Type representing a stream of message parts that make up a
 *  single message.
 *
 * If you want to recieve messages, use `message_stream::pull_type`.
 * To send messages, use `message_stream::push_type`. To see an
 * example on how to use this, see
 * [socket::recv_multimsg](\ref socket::recv_multimsg).
 */
typedef boost::coroutines2::coroutine<zmq::message_t> message_stream;


/*! A ZeroMQ socket that can recieve messages containing multiple
 *  parts as a stream.
 */
class socket : public zmq::socket_t
{
public:
    // Just use regular socket's constructors
    using socket_t::socket_t;

    /*! \brief Recieve a message that has multiple parts as a stream.
     */
    auto recv_multimsg() -> message_stream::pull_type;

    /*! \brief Send a message that has multiple parts.
     */
    auto send_multimsg(message_stream::pull_type &) -> void;
};
