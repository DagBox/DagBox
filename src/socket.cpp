#include "socket.hpp"


auto socket::recv_multimsg() -> std::vector<zmq::message_t>
{
    std::vector<zmq::message_t> messages;
    bool has_more = true;
    while (has_more) {
        zmq::message_t message;
        // recv in blocking mode should never give false
        assert(recv(&message) == true);
        has_more = message.more();
        messages.push_back(std::move(message));
    }
    return messages;
}
