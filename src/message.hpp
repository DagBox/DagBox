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
    /*! \brief Exactly one message part. */
    typedef zmq::message_t part;
    /*! \brief Zero or one message part. */
    typedef boost::optional<part> optional_part;
    /*! \brief Zero or more message parts. */
    typedef std::vector<part> many_parts;


    /*! \brief Exceptions that can be thrown by [msg](\ref msg)
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


    namespace detail
    {
        namespace protocol
        {
            std::string const name = "DGBX";
            char const version = 0x01;

            std::string const header = name + version;
        }


        enum class types
        {
            registration = 0x01,
            ping = 0x02,
            pong = 0x03,
            request = 0x04,
            reply = 0x05,
        };
        auto const type_upper_bound = static_cast<char>(types::reply);
        auto const type_lower_bound = static_cast<char>(types::registration);


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


    // Message types
    class registration;
    class ping;
    class pong;
    class request;
    class reply;

    /*! \brief Any message type.
     *
     * Represents a type that may be any of the message types. See
     * [boost::variant](http://www.boost.org/doc/libs/1_63_0/doc/html/variant.html)
     * on how to interact with this type.
     */
    typedef boost::variant<
        registration,
        ping,
        pong,
        request,
        reply
        > any_message;


    /*! \brief Read a message from message parts.
     *
     * \param parts The message parts that make up the message.
     *
     * \returns Any of the message types, depending on what is read
     * from the iterator. If no exceptions have been thrown, the
     * message is guaranteed to be valid.
     *
     * \throws exception::malformed A malformed message was recieved.
     * \throws exception::unsupported_version A message using an
     * unsupported version of the protocol was received.
     *
     * Note that this function may throw an exception before consuming
     * the iterator completely.
     */
    auto read(std::vector<zmq::message_t> && parts) -> any_message;


    /*! \brief An [InputIterator](http://en.cppreference.com/w/cpp/concept/InputIterator) that contains message parts.
     */
    typedef detail::part_source::iterator message_iterator;

    /*! \brief Convert a message into an iterator that can be used to
     *  send that message.
     *
     * Note that once a message is passed to this function, that
     * message should be considered invalid and should not be accessed
     * in any way.
     *
     * \returns A pair of iterators. The first iterator is the
     * beginning iterator, the second one is the end.
     */
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
                using namespace detail;

                auto sender       = read_optional(iter, end);
                auto sender_delim = read_part(iter, end);
                auto protocol     = read_part(iter, end);
                auto type_        = read_part(iter, end);

                header h(std::move(sender),
                         std::move(sender_delim),
                         std::move(protocol),
                         std::move(type_));

                h.validate();

                return h;
            }

            auto type() const noexcept -> enum types;

            auto type(enum types new_type) noexcept -> void;

            friend auto detail::send_header(detail::part_sink & sink,
                                            header && head) -> void;
        };
    };


    /*! \brief A service registration message.
     *
     * Workers send this message to the broker to register the service
     * they can provide.
     */
    class registration
    {
        detail::header head;
        part service_;

        registration(detail::header && head,
                     part && service);

        auto send(detail::part_sink & sink) -> void;

        template <class iterator>
        auto static read(detail::header && h, iterator & iter, iterator & end)
            -> registration {
            auto service = detail::read_part(iter, end);

            return registration(std::move(h),
                                std::move(service));
        }

        enum detail::types static const type = detail::types::registration;
    public:
        /*! \brief Create a service registration message.
         */
        auto static make(std::string const & service_name) noexcept
            -> registration;

        /*! \brief Get the service name this message is registering for.
         */
        auto service() -> std::string const;

        friend auto read(std::vector<zmq::message_t> && parts) -> any_message;
        friend auto send(registration && msg)
            -> std::tuple<message_iterator, message_iterator>;
    };


    /*! \brief A heartbeat message used to check if the
     *  recipient is alive.
     *
     * This message is typically sent by the broker to check if the
     * workers are still alive, but may be sent by the workers as
     * well.
     *
     * The recipient of the message must reply with a [pong}(\ref pong)
     * message.
     */
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

        enum detail::types static const type = detail::types::ping;
    public:
        /*! \brief Create a heartbeat message.
         */
        auto static make() noexcept -> ping;

        friend auto read(std::vector<zmq::message_t> && parts) -> any_message;
        friend auto send(ping && msg)
            -> std::tuple<message_iterator, message_iterator>;

        friend class pong;
    };


    /*! \brief A heartbeat response used to indicate that the sender
     *  is alive.
     *
     * When the broker or a worker recieves a [ping](\ref ping), they
     * must reply with this message.
     */
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

        enum detail::types static const type = detail::types::pong;
    public:
        /*\ brief Create a response for a heartbeat message.
         *
         * When a [ping](\ref ping) is recieved, it can be passed to
         * this function to create a reply for it.
         */
        auto static make(ping && p) noexcept -> pong;

        friend auto read(std::vector<zmq::message_t> && parts) -> any_message;
        friend auto send(pong && msg)
            -> std::tuple<message_iterator, message_iterator>;
    };


    /*! \brief A request for a service to do some form of work.
     */
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
            using namespace detail;

            auto service            = read_part(iter, end);
            auto client             = read_optional(iter, end);
            auto client_delimiter   = read_part(iter, end);
            auto metadata           = read_many(iter, end);
            auto metadata_delimiter = read_part(iter, end);
            auto data               = read_many(iter, end);

            return request(std::move(head),
                           std::move(service),
                           std::move(client),
                           std::move(client_delimiter),
                           std::move(metadata),
                           std::move(metadata_delimiter),
                           std::move(data));
        }

        enum detail::types static const type = detail::types::request;
    public:
        /*! \brief Create a new request.
         *
         * \param service_name The name of the service the request
         * will be sent to.
         *
         * \param metadata_parts Metadata about the request. These
         * metadata parts will be returned back to the client along
         * with the reply without any modifications. This can be used,
         * for example, to match requests and replies when sending
         * multiple requests at once.
         *
         * \param data_parts The data of the request. The format of
         * these parts depend on the service.
         */
        auto static make(std::string const & service_name,
                         many_parts && metadata_parts,
                         many_parts && data_parts)
            -> request;

        /*! \brief Get the metadata of the request.
         *
         * The reference returned by this function is valid as long as
         * the object it is called on is. In other words, the
         * reference should not be used after the message object is
         * moved (`std::move`) or destructed.
         */
        auto inline metadata() -> many_parts & {
            return metadata_;
        }
        /*! \brief Get the data of the request.
         *
         * The reference returned by this function is valid as long as
         * the object it is called on is. In other words, the
         * reference should not be used after the message object is
         * moved (`std::move`) or destructed.
         */
        auto inline data() -> many_parts & {
            return data_;
        }

        friend auto read(std::vector<zmq::message_t> && parts) -> any_message;
        friend auto send(request && msg)
            -> std::tuple<message_iterator, message_iterator>;

        friend class reply;
    };


    /*! \brief A reply from a service containing the result of requested work.
     */
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
            using namespace detail;

            auto client             = read_optional(iter, end);
            auto client_delimiter   = read_part(iter, end);
            auto metadata           = read_many(iter, end);
            auto metadata_delimiter = read_part(iter, end);
            auto data               = read_many(iter, end);

            return reply(std::move(head),
                         std::move(client),
                         std::move(client_delimiter),
                         std::move(metadata),
                         std::move(metadata_delimiter),
                         std::move(data));
        }

        enum detail::types static const type = detail::types::reply;
    public:
        /*! \brief Create a response for a request.
         */
        auto static make(request && r) -> reply;

        /*! \brief Get the metadata of the request.
         *
         * The reference returned by this function is valid as long as
         * the object it is called on is. In other words, the
         * reference should not be used after the message object is
         * moved (`std::move`) or destructed.
         */
        auto inline metadata() -> many_parts & {
            return metadata_;
        }
        /*! \brief Get the data of the request.
         *
         * The reference returned by this function is valid as long as
         * the object it is called on is. In other words, the
         * reference should not be used after the message object is
         * moved (`std::move`) or destructed.
         */
        auto inline data() -> many_parts & {
            return data_;
        }

        friend auto read(std::vector<zmq::message_t> && parts) -> any_message;
        friend auto send(reply && msg)
            -> std::tuple<message_iterator, message_iterator>;
    };
};
