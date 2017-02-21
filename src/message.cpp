#include <cstring>
#include <zmq.hpp>
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



message::header::header(zmq::message_t && init_part,
                        zmq::message_t && protocol_part,
                        zmq::message_t && type_part)
    : init_part(std::move(init_part)),
      protocol_part(std::move(protocol_part)),
      type_part(std::move(type_part))
{
    validate_or_throw();
}


auto message::header::make(enum type type_) -> header
{
    return header(zmq::message_t(),
                  make_protocol_part(),
                  make_type_part(type_));
}


auto message::header::make(std::vector<zmq::message_t> && messages) -> header
{
    if (messages.size() < 3) {
        throw message::exception::malformed(
            "Wrong protocol, or header missing parts");
    }

    zmq::message_t & init_part = messages[0];
    zmq::message_t & protocol_part = messages[1];
    zmq::message_t & type_part = messages[2];

    return header(std::move(init_part),
                  std::move(protocol_part),
                  std::move(type_part));
}



auto message::header::validate_or_throw() -> void
{
    using namespace message::exception;

    if (init_part.size() != 0) {
        throw malformed("Multi-part message doesn't start with empty part");
    }

    if (protocol_part.size()
        != (message::protocol::name.size()
            + sizeof(message::protocol::version))) {
        throw malformed("Protocol header part is malformed");
    }
    if (memcmp(protocol_part.data(),
               message::protocol::name.data(),
               message::protocol::name.size()) != 0) {
        throw malformed("Protocol header is invalid");
    }
    if (*protocol_part.data<uint8_t>() != message::protocol::version) {
        throw unsupported_version("Message uses an unsupported "
                                  "version of the protocol");
    }

    if (type_part.size() != 1) {
        throw malformed("Message type part is malformed");
    }
    using namespace message::detail;
    auto t = *type_part.data<char>();
    if (!(type_lower_bound <= t && t <= type_upper_bound)) {
        throw malformed("Invalid message type");
    }
}
