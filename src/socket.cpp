#include "socket.hpp"


auto socket::recv_multimsg() -> message_stream::pull_type
{
    return message_stream::pull_type(
        [&](message_stream::push_type & sink){
            bool has_more = true;
            while (has_more) {
                zmq::message_t message;
                // recv in blocking mode should never give false
                assert(recv(&message) == true);
                has_more = getsockopt<int64_t>(ZMQ_RCVMORE) == 1;
                sink(message);
            }
        });
}



auto socket::send_multimsg(message_stream::pull_type & parts) -> void
{
    while (parts) {
        auto part = parts.get();
        parts();
        if (parts) { // More parts to send
            send(part, ZMQ_SNDMORE);
        } else {
            send(part);
        }
    }
}
