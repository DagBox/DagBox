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
    auto read_part(iterator & iter, iterator & end) -> part {
        if (iter == end) {
            throw exception::malformed("Expected message part is missing");
        }
        return *(iter++);
    }

    template <class iterator>
    auto read_optional(iterator & iter, iterator & end) -> optional_part {
        // If there are no parts left, no optional part
        if (iter == end) {
            return boost::none;
        }
        // If the first part is empty, no optional part
        part first = *iter;
        if (first.size() == 0) {
            return boost::none;
        }
        // If the first part is not empty, and there are no parts
        // left, then we found the optional part
        ++iter;
        if (iter == end) {
            return first;
        }
        // If the first part is not empty and the second part is, then
        // we found the optional part
        part second = *iter;
        if (second.size() == 0) {
            return first;
        }

        // If both the first and the second part are not empty, then
        // we weren't sent an optional part at all
        throw exception::malformed("Expected optional message part "
                                   "is malformed");
    }

    template <class iterator>
    auto read_many(iterator & iter, iterator & end) -> many_parts {
        many_parts parts;
        while (iter != end && (*iter).size() != 0) {
            parts.push_back(*(iter++));
        }
        return parts;
    }


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
        auto static read(iterator & iter, iterator & end) -> header {
            auto sender       = read_optional(iter, end);
            auto sender_delim = read_part(iter, end);
            auto protocol     = read_part(iter, end);
            auto type_        = read_part(iter, end);

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
        auto static read(header && h, iterator & iter, iterator & end)
            -> registration {
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
        auto static read(header && h, iterator & iter, iterator & end)
            -> ping {
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
        auto static read(header && h, iterator & iter, iterator & end)
            -> pong {
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
        auto static read(header && head,
                         iterator & iter,
                         iterator & end)
            -> request {
            auto service            = read_part(iter, end);
            auto client             = read_optional(iter, end);
            auto client_delimiter   = read_part(iter, end);
            auto metadata           = read_many(iter, end);
            auto metadata_delimiter = read_part(iter, end);
            auto data               = read_many(iter, end);

            return request(head, service, client, client_delimiter,
                           metadata, metadata_delimiter, data);
        }

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
        auto static read(header && head,
                         iterator & iter,
                         iterator & end)
            -> reply {
            auto client             = read_optional(iter, end);
            auto client_delimiter   = read_part(iter, end);
            auto metadata           = read_many(iter, end);
            auto metadata_delimiter = read_part(iter, end);
            auto data               = read_many(iter, end);

            return reply(head, client, client_delimiter,
                         metadata, metadata_delimiter, data);
        }

        enum types static const type = types::reply;
    };


    template <class iterator>
    auto read(iterator & iter, iterator & end)
        -> any_message {
        header h = header::read(iter, end);

        switch (h.type()) {
        case types::ping:
            return ping::read(h, iter, end);
            break;
        case types::pong:
            return pong::read(h, iter, end);
            break;
        case types::registration:
            return registration::read(h, iter, end);
            break;
        case types::request:
            return request::read(h, iter, end);
            break;
        case types::reply:
            return reply::read(h, iter, end);
            break;
        }
    }
};
