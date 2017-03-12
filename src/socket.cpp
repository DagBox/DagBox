#include "socket.hpp"


auto socket::recv_multimsg() -> std::vector<zmq::message_t>
{
    std::vector<zmq::message_t> messages;
    bool has_more = true;
    while (has_more) {
        zmq::message_t message;
        auto recv_size = recv(&message);
        if (recv_size == 0) { // recv timed out
            // recv should only timeout if we recieved no message at all
            assert(messages.size() == 0);
            return messages;
        }
        has_more = message.more();
        messages.push_back(std::move(message));
    }
    return messages;
}
