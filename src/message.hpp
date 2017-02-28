#pragma once

#include <vector>
#include <tuple>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/coroutine2/coroutine.hpp>
#include "socket.hpp"

// Allow ADL-selected overloads on begin and end for both user defined
// types (i.e. boost coroutines) and STL types
using std::begin;
using std::end;


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
        std::string const name = "DGBX";
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


    namespace detail
    {
        template <class iterator>
        auto read_part(iterator & iter, iterator & end) -> part {
            if (iter == end) {
                throw exception::malformed("Expected message part is missing");
            }
            return std::move(*(iter++));
        }

        template <class iterator>
        auto read_optional(iterator & iter, iterator & end) -> optional_part {
            // If there are no parts left, no optional part
            if (iter == end) {
                return boost::none;
            }
            // If the first part is empty, no optional part
            part first = std::move(*iter);
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
            part second = std::move(*iter);
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
                parts.push_back(std::move(*(iter++)));
            }
            return parts;
        }

        class header;


        // We want to hide the read methods of the message
        // classes. But when we make them private, the `msg::read`
        // function can't access them, and the classes can't friend
        // `msg::read` because it is a template function, and the type
        // is unknown until the point where `msg::read` is called.  To
        // get around this, we create this dummy class, which the
        // message classes can friend.
        class reader
        {
            template <class message, class iterator>
            auto inline static read(header && head,
                                    iterator & iter,
                                    iterator & end)
                -> message {
                return message::read(std::move(head), iter, end);
            }
        };


    };
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


    namespace detail
    {
        typedef boost::coroutines2::coroutine<part> part_stream;

        typedef part_stream::push_type part_sink;
        typedef part_stream::pull_type part_source;

        // Placed here rather than inside the header class to avoid
        // exposing it as a part of our public API. Otherwise, we'd
        // have to make it a private function and would have to make
        // all message classes friend.
        auto send_header(part_sink & sink, header && head) -> void;

        auto inline send_section(part_sink & sink, part & p) -> void
        {
            sink(p);
        }

        auto inline send_section(part_sink & sink, optional_part & p) -> void
        {
            if (p) {
                sink(*p);
            }
        }

        auto inline send_section(part_sink & sink, many_parts & ps) -> void
        {
            for (auto & p : ps) {
                sink(p);
            }
        }
    };

    typedef detail::part_source::iterator message_iterator;

    template <class message>
    auto send(message && msg)
        -> std::tuple<message_iterator, message_iterator> {
        using namespace detail;

        part_source source([&](detail::part_sink & sink){
            send_header(sink, std::move(msg.head));

            msg.send(sink);
        });
        return std::make_tuple(begin(source), end(source));
    }


    namespace detail
    {
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

            friend auto detail::send_header(detail::part_sink & sink,
                                            header && head) -> void;
        };
    };


    class registration
    {
        detail::header head;
        optional_part service;

        registration(detail::header && head,
                     optional_part && service);

        auto send(detail::part_sink & sink) -> void;

        template <class iterator>
        auto static read(detail::header && h, iterator & iter, iterator & end)
            -> registration {
            auto service = read_optional(iter);

            return registration(h, service);
        }
    public:
        auto static make(std::string const & service_name) noexcept
            -> registration;

        enum types static const type = types::registration;

        friend auto send(registration && msg)
            -> std::tuple<message_iterator, message_iterator>;

        friend class detail::reader;
    };


    class ping
    {
        detail::header head;

        ping(detail::header && head);

        auto send(detail::part_sink & sink) -> void;

        template <class iterator>
        auto static read(detail::header && h, iterator & iter, iterator & end)
            -> ping {
            return ping(std::move(h));
        }
    public:
        auto static make() noexcept -> ping;

        enum types static const type = types::ping;

        friend class detail::reader;
        friend auto send(ping && msg)
            -> std::tuple<message_iterator, message_iterator>;

        friend class pong;
    };


    class pong
    {
        detail::header head;

        pong(detail::header && head);

        auto send(detail::part_sink & sink) -> void;

        template <class iterator>
        auto static read(detail::header && h, iterator & iter, iterator & end)
            -> pong {
            return pong(std::move(h));
        }
    public:
        auto static make(ping && p) noexcept -> pong;

        friend class detail::reader;
        friend auto send(pong && msg)
            -> std::tuple<message_iterator, message_iterator>;

        enum types static const type = types::pong;
    };


    class request
    {
        detail::header head;
        part          service;
        optional_part client;
        part          client_delimiter;
        many_parts    metadata_;
        part          metadata_delimiter;
        many_parts    data_;

        request(detail::header && head,
                part          && service,
                optional_part && client,
                part          && client_delimiter,
                many_parts    && metadata,
                part          && metadata_delimiter,
                many_parts    && data);

        auto send(detail::part_sink & sink) -> void;

        template <class iterator>
        auto static read(detail::header && head,
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
    public:
        auto static make(std::string const & service_name,
                         many_parts && metadata_parts,
                         many_parts && data_parts)
            -> request;

        auto inline metadata() -> many_parts & {
            return metadata_;
        }
        auto inline data() -> many_parts & {
            return data_;
        }

        enum types static const type = types::request;

        friend class detail::reader;
        friend auto send(request && msg)
            -> std::tuple<message_iterator, message_iterator>;

        friend class reply;
    };


    class reply
    {
        detail::header head;
        optional_part client;
        part          client_delimiter;
        many_parts    metadata_;
        part          metadata_delimiter;
        many_parts    data_;

        reply(detail::header && head,
              optional_part && client,
              part          && client_delimiter,
              many_parts    && metadata,
              part          && metadata_delimiter,
              many_parts    && data);

        auto send(detail::part_sink & sink) -> void;

        template <class iterator>
        auto static read(detail::header && head,
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
    public:
        auto static make(request && r) -> reply;

        auto inline metadata() -> many_parts & {
            return metadata_;
        }
        auto inline data() -> many_parts & {
            return data_;
        }

        enum types static const type = types::reply;

        friend class detail::reader;
        friend auto send(reply && msg)
            -> std::tuple<message_iterator, message_iterator>;
    };


    template <class iterator>
    auto read(iterator & iter, iterator & end)
        -> any_message {
        using namespace detail;

        header h = header::read(iter, end);

        switch (h.type()) {
        case types::ping:
            return reader::read<ping>(std::move(h), iter, end);
            break;
        case types::pong:
            return reader::read<pong>(std::move(h), iter, end);
            break;
        case types::registration:
            return reader::read<registration>(std::move(h), iter, end);
            break;
        case types::request:
            return reader::read<request>(std::move(h), iter, end);
            break;
        case types::reply:
            return reader::read<reply>(std::move(h), iter, end);
            break;
        }
    }
};
