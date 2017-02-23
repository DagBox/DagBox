#pragma once

#include <vector>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include "socket.hpp"

/*! \file message.hpp
 * Helpers for handling messages.
 */



/*! \brief Declare an exception class.
 *
 * Declares an exception class with the name `name` that inherits from
 * `base` class. The declared class will retain all constructors of
 * the base class.
 */
#define EXCEPTION(name, base)                        \
    class name                                       \
        : public base                                \
    {using base::base;}



/*! \brief Helpers for handling messages.
 */
namespace msg
{
    namespace protocol
    {
        std::string const name = {'D', 'G', 'B', 'X'};
        char const version = 0x01;

        std::string const header = name + version;
    }

    /*! \brief Exceptions that can be thrown by [message](\ref message)
     *  namespace.
     */
    namespace exception
    {
        using std::runtime_error;

        /*! \brief A malformed message was recieved.
         *
         * The recieved message was from a completely different
         * protocol. This suggests that something that doesn't support
         * our protocol has connected to our socket.
         */
        EXCEPTION(malformed, runtime_error);

        /*! \brief A message using an unsupported version of the
         *  protocol was recieved.
         */
        EXCEPTION(unsupported_version, runtime_error);
    }


    /*! Types of messages. */
    enum class types
    {
        registration = 0x01,
        ping = 0x02,
        pong = 0x03,
        request = 0x04,
        reply = 0x05,
    };
    namespace detail
    {
        auto const type_upper_bound = static_cast<char>(types::reply);
        auto const type_lower_bound = static_cast<char>(types::registration);
    }


    typedef zmq::message_t part;
    typedef boost::optional<part> optional_part;
    typedef std::vector<part> many_parts;


    template <class iterator>
    auto read_part(iterator & iter) -> part;

    template <class iterator>
    auto read_optional(iterator & iter) -> optional_part;

    template <class iterator>
    auto read_many(iterator & iter) -> many_parts;


    // Specialized message types
    class registration;
    class ping;
    class pong;
    class request;
    class reply;


    typedef boost::variant<
        registration,
        ping,
        pong,
        request,
        reply
        > any_message;


    class header
    {
        auto static make_protocol_part() noexcept -> part;

        auto static make_type_part(enum types type_) noexcept -> part;

        optional_part sender;
        part sender_delimiter;
        part protocol;
        part type_;

        auto validate() -> void;

        header(optional_part && sender,
               part && sender_delimiter,
               part && protocol,
               part && type_);
    public:
        auto static make(enum types type_) noexcept -> header;

        template <class iterator>
        auto static read(iterator & iter) -> header {
            auto sender       = read_optional(iter);
            auto sender_delim = read_part(iter);
            auto protocol     = read_part(iter);
            auto type_        = read_part(iter);

            header h(sender,
                     sender_delim,
                     protocol,
                     type_);

            h.validate();

            return h;
        }

        auto type() const noexcept -> enum types;

        auto type(enum types new_type) noexcept -> void;
    };


    class registration
    {
        header head;
        optional_part service;

        registration(header && head,
                     optional_part && service);
    public:
        auto static make(std::string const & service_name) noexcept
            -> registration;

        template <class iterator>
        auto static read(header && h, iterator & iter) -> registration {
            auto service = read_optional(iter);

            return registration(h, service);
        }

        enum types static const type = types::registration;
    };


    class ping
    {
        header head;

        ping(header && head);
    public:
        auto static make() noexcept -> ping;

        template <class iterator>
        auto static read(header && h, iterator & iter) -> ping {
            return ping(std::move(h));
        }

        enum types static const type = types::ping;

        friend class pong;
    };


    class pong
    {
        header head;

        pong(header && head);
    public:
        auto static make(ping && p) noexcept -> pong;

        template <class iterator>
        auto static read(header && h, iterator & iter_) -> pong {
            return pong(std::move(h));
        }

        enum types static const type = types::pong;
    };


    class request
    {
        header        head;
        part          service;
        optional_part client;
        part          client_delimiter;
        many_parts    metadata;
        part          metadata_delimiter;
        many_parts    data;

        request(header        && head,
                part          && service,
                optional_part && client,
                part          && client_delimiter,
                many_parts    && metadata,
                part          && metadata_delimiter,
                many_parts    && data);
    public:
        auto static make(std::string const & service_name,
                         many_parts && metadata_parts,
                         many_parts && data_parts)
            -> request;
        
        template <class iterator>
        auto static read(header && head, iterator & iter) -> request;

        enum types static const type = types::request;
    };


    class reply
    {
        optional_part client;
        part          client_delimiter;
        many_parts    metadata;
        part          metadata_delimiter;
        many_parts    data;

        reply(header        && head,
              optional_part && client,
              part          && client_delimiter,
              many_parts    && metadata,
              part          && metadata_delimiter,
              many_parts    && data);
    public:
        auto static make(request && r) -> reply;

        template <class iterator>
        auto static read(header && head, iterator & iter) -> reply;

        enum types static const type = types::reply;
    };


    template <class iterator>
    auto read(iterator & iter)
        -> any_message {
        header h = header::read(iter);

        switch (h.type()) {
        case types::ping:
            return ping::read(h, iter);
            break;
        case types::pong:
            return pong::read(h, iter);
            break;
        case types::registration:
            return registration::read(h, iter);
            break;
        case types::request:
            return request::read(h, iter);
            break;
        case types::reply:
            return reply::read(h, iter);
            break;
        }
    }
};
