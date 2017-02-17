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
    : init_part(zmq::message_t()),
      protocol_part(make_protocol_part()),
      type_part(make_type_part(type_))
{}


template <class iterator>
message::header::header(iterator it)
    : init_part(it++),
      protocol_part(it++),
      type_part(it++)
{
    // Validation
    if (init_part.size() != 0) {
        throw message::exception::malformed("Non-empty message start");
    }
}
