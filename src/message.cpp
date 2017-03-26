/*
  Copyright 2017 Kaan Genç

  This file is part of DagBox.

  DagBox is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  DagBox is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with DagBox.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <cstring>
#include <zmq.hpp>
#include "message.hpp"
using namespace msg;
using namespace detail;



auto msg::read(std::vector<zmq::message_t> && parts) -> any_message
{
    if (parts.size() == 0) {
        throw std::logic_error("Unable to read empty message");
    }

    auto iter = begin(parts);
    auto end_ = end(parts);

    header h = header::read(iter, end_);

    switch (h.type()) {
    case types::ping:
        return ping::read(std::move(h), iter, end_);
        break;
    case types::pong:
        return pong::read(std::move(h), iter, end_);
        break;
    case types::registration:
        return registration::read(std::move(h), iter, end_);
        break;
    case types::request:
        return request::read(std::move(h), iter, end_);
        break;
    case types::reply:
        return reply::read(std::move(h), iter, end_);
        break;
    case types::reconnect:
        return reconnect::read(std::move(h), iter, end_);
        break;
    }

    // The compiler can't recognise that the switch above will always
    // return when all cases are handled, and gives an unnecessary
    // warning about the control reaching the end of the
    // function. Adding this exception to silence that warning.
    throw exception::runtime_error("We should never hit this exception");
}


//////////////////// Header

auto header::make_protocol_part() noexcept -> part
{
    return part(protocol::header.data(),
                protocol::header.size());
}


auto header::make_type_part(enum types type_) noexcept -> part
{
    return part(&type_, sizeof(type_));
}


header::header(optional_part && address_,
               part && address_delim,
               part && protocol_part,
               part && type_part)
    : address_(std::move(address_)),
      address_delim(std::move(address_delim)),
      protocol(std::move(protocol_part)),
      type_(std::move(type_part))
{
    validate();
}


auto header::make(enum types type_) noexcept -> header
{
    return header(boost::none,
                  part(),
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
    if (protocol.data<uint8_t>()[protocol::name.size()] != protocol::version) {
        throw unsupported_version("Message uses an unsupported "
                                  "version of the protocol");
    }

    if (type_.size() != sizeof(enum types)) {
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


auto header::address() const noexcept -> boost::optional<msg::address>
{
    if (!address_) {
        return boost::none;
    } else {
        return std::string(address_->data<char>(), address_->size());
    }
}


auto header::address(std::string const & addr) -> void
{
    address_ = part(addr.data(), addr.size());
}


auto header::send(part_sink & sink) -> void
{
    send_section(sink, address_);
    send_section(sink, address_delim);
    send_section(sink, protocol);
    send_section(sink, type_);
}


//////////////////// Registration

registration::registration(header && head,
                           part && service)
    : head(std::move(head)),
      service_(std::move(service))
{}


auto registration::make(std::string const & service_name) noexcept
    -> registration
{
    return registration(header::make(types::registration),
                        part(service_name.data(),
                             service_name.size()));
}


auto registration::send(detail::part_sink & sink)
    -> void
{
    detail::send_section(sink, service_);
}



//////////////////// Ping

ping::ping(header && head)
    : head(std::move(head))
{}


auto ping::make() noexcept -> ping
{
    return ping(header::make(ping::type));
}


auto ping::send(detail::part_sink &) -> void
{
    // We do nothing here, because ping messages don't have any
    // parts other than their header
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


auto pong::send(detail::part_sink &) -> void
{
    // We do nothing here, because pong messages don't have any parts
    // other than their header
}


//////////////////// Request

request::request(header        && head,
                 part          && service_,
                 optional_part && client_,
                 part          && client_delimiter,
                 many_parts    && metadata,
                 part          && metadata_delimiter,
                 many_parts    && data)
    : head(std::move(head)),
      service_(std::move(service_)),
      client_(std::move(client_)),
      client_delimiter(std::move(client_delimiter)),
      metadata_(std::move(metadata)),
      metadata_delimiter(std::move(metadata_delimiter)),
      data_(std::move(data))
{}


auto request::make(std::string const & service_name,
                   many_parts && metadata_parts,
                   many_parts && data_parts)
    -> request
{
    return request(
        header::make(request::type),
        part(service_name.data(), service_name.size()),
        boost::none,
        part(),
        std::move(metadata_parts),
        part(),
        std::move(data_parts));
}


auto request::send(detail::part_sink & sink) -> void
{
    using namespace detail;

    send_section(sink, service_);
    send_section(sink, client_);
    send_section(sink, client_delimiter);
    send_section(sink, metadata_);
    send_section(sink, metadata_delimiter);
    send_section(sink, data_);
}



//////////////////// Reply

reply::reply(header        && head,
             optional_part && client_,
             part          && client_delimiter,
             many_parts    && metadata,
             part          && metadata_delimiter,
             many_parts    && data)
    : head(std::move(head)),
      client_(std::move(client_)),
      client_delimiter(std::move(client_delimiter)),
      metadata_(std::move(metadata)),
      metadata_delimiter(std::move(metadata_delimiter)),
      data_(std::move(data))
{}


auto reply::make(msg::request && r) -> reply
{
    auto rep = reply(std::move(r.head),
                 std::move(r.client_),
                 std::move(r.client_delimiter),
                 std::move(r.metadata_),
                 std::move(r.metadata_delimiter),
                 std::move(r.data_));
    rep.head.type(reply::type);
    return rep;
}


auto reply::send(detail::part_sink & sink) -> void
{
    using namespace detail;

    send_section(sink, client_);
    send_section(sink, client_delimiter);
    send_section(sink, metadata_);
    send_section(sink, metadata_delimiter);
    send_section(sink, data_);
}



//////////////////// Reconnect

reconnect::reconnect(detail::header && head)
    : head(std::move(head))
{}


auto reconnect::make(msg::ping && p) noexcept -> reconnect
{
    auto rec = reconnect(std::move(p.head));
    rec.head.type(reconnect::type);
    return rec;
}


auto reconnect::send(detail::part_sink &) -> void
{
    // Reconnect messages only have a header, nothing to do here
}
