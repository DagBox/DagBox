#include <cstring>
#include <zmq.hpp>
#include "message.hpp"
using namespace msg;



//////////////////// Header

auto header::make_protocol_part() noexcept -> part
{
    return zmq::message_t(protocol::header.data(),
                          protocol::header.size());
}


auto header::make_type_part(enum types type_) noexcept -> part
{
    return zmq::message_t(&type_, sizeof(type_));
}


header::header(optional_part && sender,
               part && sender_delimiter,
               part && protocol_part,
               part && type_part)
    : sender(std::move(sender)),
      sender_delimiter(std::move(sender_delimiter)),
      protocol(std::move(protocol_part)),
      type_(std::move(type_part))
{
    validate();
}


auto header::make(enum types type_) noexcept -> header
{
    return header(boost::none,
                  zmq::message_t(),
                  make_protocol_part(),
                  make_type_part(type_));
}


auto header::validate() -> void
{
    using namespace exception;

    if (protocol.size()
        != (protocol::name.size()
            + sizeof(protocol::version))) {
        throw malformed("Protocol header part is malformed");
    }
    if (memcmp(protocol.data(),
               protocol::name.data(),
               protocol::name.size()) != 0) {
        throw malformed("Protocol header is invalid");
    }
    if (*protocol.data<uint8_t>() != protocol::version) {
        throw unsupported_version("Message uses an unsupported "
                                  "version of the protocol");
    }

    if (type_.size() != 1) {
        throw malformed("Message type part is malformed");
    }
    using namespace detail;
    auto t = *type_.data<char>();
    if (!(type_lower_bound <= t && t <= type_upper_bound)) {
        throw malformed("Invalid message type");
    }
}


auto header::type() const noexcept -> enum types
{
    return *(type_.data<enum types>());
}


auto header::type(enum types new_type) noexcept -> void
{
    *(type_.data<enum types>()) = new_type;
}



//////////////////// Registration

registration::registration(header && head,
                           optional_part && service)
    : head(std::move(head)),
      service(std::move(service))
{}


auto registration::make(std::string const & service_name) noexcept
    -> registration
{
    return registration(header::make(types::registration),
                        zmq::message_t(service_name.data(),
                                       service_name.size()));
}



//////////////////// Ping

ping::ping(header && head)
    : head(std::move(head))
{}


auto ping::make() noexcept -> ping
{
    return ping(header::make(ping::type));
}



//////////////////// Pong

pong::pong(header && head)
    : head(std::move(head))
{}


auto pong::make(ping && p) noexcept -> pong
{
    auto pong_ = pong(std::move(p.head));
    pong_.head.type(pong::type);
    return pong_;
}



//////////////////// Request

request::request(header        && head,
                 part          && service,
                 optional_part && client,
                 part          && client_delimiter,
                 many_parts    && metadata,
                 part          && metadata_delimiter,
                 many_parts    && data)
    : head(std::move(head)),
      service(std::move(service)),
      client(std::move(client)),
      client_delimiter(std::move(client_delimiter)),
      metadata(std::move(metadata)),
      metadata_delimiter(std::move(metadata_delimiter)),
      data(std::move(data))
{}
