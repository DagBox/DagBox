#include <cstring>
#include "message.hpp"


auto message::header::make_protocol_part() -> zmq::message_t
{
    return zmq::message_t(message::protocol::header.data(),
                          message::protocol::header.size());
}

auto message::header::make_type_part(enum type type_) -> zmq::message_t
{
    return zmq::message_t(&type_, sizeof(type_));
}



message::header::header(enum type type_)
{
    messages.push_back(zmq::message_t());
    messages.push_back(make_protocol_part());
    messages.push_back(make_type_part(type_));
    #ifdef DEBUG
    // Validate the message header to make sure we generate a valid
    // message. This normally shouldn't be necessary, so we skip it
    // outside debug builds for performance reasons.
    validate_or_throw();
    #endif
}


message::header::header(std::vector<zmq::message_t> && messages)
    : messages(std::move(messages))
{
    validate_or_throw();
}



auto message::header::validate_or_throw() -> void
{
    using namespace message::exception;

    if (messages.size() < 3) {
        throw malformed("Wrong protocol, or header missing parts");
    }

    if (init_part().size() != 0) {
        throw malformed("Multi-part message doesn't start with empty part");
    }

    if (protocol_part().size()
        != (message::protocol::name.size()
            + sizeof(message::protocol::version))) {
        throw malformed("Protocol header part is malformed");
    }
    if (memcmp(protocol_part().data(),
               message::protocol::name.data(),
               message::protocol::name.size()) != 0) {
        throw malformed("Protocol header is invalid");
    }
    if (*protocol_part().data<uint8_t>() != message::protocol::version) {
        throw unsupported_version("Message uses an unsupported "
                                  "version of the protocol");
    }

    if (type_part().size() != 1) {
        throw malformed("Message type part is malformed");
    }
    using namespace message::detail;
    auto t = *type_part().data<char>();
    if (!(type_lower_bound <= t && t <= type_upper_bound)) {
        throw malformed("Invalid message type");
    }
}
