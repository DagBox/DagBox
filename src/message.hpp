/*
  Copyright 2017 Kaan Genç, Melis Narin Kaya

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
#pragma once

#include <vector>
#include <tuple>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include "exception.hpp"
#include "socket.hpp"

// Allow ADL-selected overloads on begin and end for both user defined
// types (i.e. boost coroutines) and STL types
using std::begin;
using std::end;


/*! \file message.hpp
 * Helpers for handling messages.
 */




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

    /*! \brief A 0mq address. */
    typedef std::string address;


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
            reconnect = 0x06,
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
                return std::move(first);
            }
            // If the first part is not empty and the second part is, then
            // we found the optional part
            part second = std::move(*iter);
            if (second.size() == 0) {
                return std::move(first);
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


        typedef std::vector<part> part_sink;

        auto inline send_section(part_sink & sink, part & p) -> void
        {
            sink.push_back(std::move(p));
        }

        auto inline send_section(part_sink & sink, optional_part & p) -> void
        {
            if (p) {
                sink.push_back(std::move(*p));
            }
        }

        auto inline send_section(part_sink & sink, many_parts & ps) -> void
        {
            for (auto & p : ps) {
                sink.push_back(std::move(p));
            }
        }
    };


    // Message types
    class registration;
    class ping;
    class pong;
    class request;
    class reply;
    class reconnect;

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
        reply,
        reconnect
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


    /*! \brief Some form of a container which can be iterated on to recieve message parts.
     *
     * `begin` and `end` functions can be called on this
     * pseudo-container to recieve
     * [InputIterator](http://en.cppreference.com/w/cpp/concept/InputIterator)s
     * that contain message parts.
     */
    typedef detail::part_sink part_source;

    namespace detail
    {
        // The compilers don't seem to like friend'ing template
        // functions. So we create a dummy class here to make it
        // easier for message classes to friend the send function.
        struct sender
        {
            template <class message>
            auto static inline send(message & msg, detail::part_sink & sink) -> void {
                msg.head.send(sink);
                msg.send(sink);
            }
        };
    }


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
        -> part_source {
        using namespace detail;

        part_sink sink;
        sender::send(msg, sink);
        return sink;
    }


    namespace detail
    {
        class header
        {
            auto static make_protocol_part() noexcept -> part;

            auto static make_type_part(enum types type_) noexcept -> part;

            optional_part address_;
            part address_delim;
            part protocol;
            part type_;

            auto validate() -> void;

            header(optional_part && address_,
                   part && address_delim,
                   part && protocol,
                   part && type_);

            auto send(detail::part_sink & sink) -> void;
        public:
            auto static make(enum types type_) noexcept -> header;

            template <class iterator>
            auto static read(iterator & iter, iterator & end) -> header {
                using namespace detail;

                auto address_      = read_optional(iter, end);
                auto address_delim = read_part(iter, end);
                auto protocol      = read_part(iter, end);
                auto type_         = read_part(iter, end);

                header h(std::move(address_),
                         std::move(address_delim),
                         std::move(protocol),
                         std::move(type_));

                h.validate();

                return h;
            }

            auto type() const noexcept -> enum types;
            auto type(enum types new_type) noexcept -> void;
            auto address() const noexcept -> boost::optional<msg::address>;
            auto address(msg::address const & addr) -> void;

            friend class sender;
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
         *
         * \param addr The address of the broker.
         *
         * \param service_name The name of the service the worker can
         * provide.
         */
        auto static make(std::string const & service_name) noexcept
            -> registration;

        /*! \brief Get the service name this message is registering for.
         */
        auto service() -> std::string const;

        auto inline address() const noexcept -> boost::optional<msg::address> {
            return head.address();
        }

        friend auto read(std::vector<zmq::message_t> && parts) -> any_message;
        friend class detail::sender;
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
        auto static read(detail::header && h, iterator &, iterator &)
            -> ping {
            return ping(std::move(h));
        }

        enum detail::types static const type = detail::types::ping;
    public:
        /*! \brief Create a heartbeat message.
         *
         * \param addr The address of the broker.
         */
        auto static make() noexcept -> ping;

        auto inline address() const noexcept -> boost::optional<msg::address> {
            return head.address();
        }

        friend auto read(std::vector<zmq::message_t> && parts) -> any_message;
        friend class detail::sender;
        friend class pong;
        friend class reconnect;
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
        auto static read(detail::header && h, iterator &, iterator &)
            -> pong {
            return pong(std::move(h));
        }

        enum detail::types static const type = detail::types::pong;
    public:
        /*! \brief Create a response for a heartbeat message.
         *
         * When a [ping](\ref ping) is recieved, it can be passed to
         * this function to create a reply for it.
         */
        auto static make(ping && p) noexcept -> pong;

        auto inline address() const noexcept -> boost::optional<msg::address> {
            return head.address();
        }

        friend auto read(std::vector<zmq::message_t> && parts) -> any_message;
        friend class detail::sender;
    };


    /*! \brief A request for a service to do some form of work.
     */
    class request
    {
        detail::header head;
        part          service_;
        optional_part client_;
        part          client_delimiter;
        many_parts    metadata_;
        part          metadata_delimiter;
        many_parts    data_;

        request(detail::header && head,
                part           && service_,
                optional_part  && client_,
                part           && client_delimiter,
                many_parts     && metadata,
                part           && metadata_delimiter,
                many_parts     && data);

        auto send(detail::part_sink & sink) -> void;

        template <class iterator>
        auto static read(detail::header && head,
                         iterator & iter,
                         iterator & end)
            -> request {
            using namespace detail;

            auto service_           = read_part(iter, end);
            auto client_            = read_optional(iter, end);
            auto client_delimiter   = read_part(iter, end);
            auto metadata           = read_many(iter, end);
            auto metadata_delimiter = read_part(iter, end);
            auto data               = read_many(iter, end);

            return request(std::move(head),
                           std::move(service_),
                           std::move(client_),
                           std::move(client_delimiter),
                           std::move(metadata),
                           std::move(metadata_delimiter),
                           std::move(data));
        }

        enum detail::types static const type = detail::types::request;
    public:
        /*! \brief Create a new request.
         *
         * \param addr The address of the broker.
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

        auto service() -> std::string const;

        auto inline address() const noexcept -> boost::optional<msg::address> {
            return head.address();
        }

        auto inline address(msg::address const & addr) -> void {
            head.address(addr);
        }

        auto inline client() const noexcept -> boost::optional<msg::address> {
            if (client_) {
                return std::string(client_->data<char>(), client_->size());
            } else {
                return boost::none;
            }
        }

        auto inline client(msg::address const & addr) noexcept -> void {
            client_ = part(addr.data(), addr.size());
        }

        friend auto read(std::vector<zmq::message_t> && parts) -> any_message;
        friend class detail::sender;
        friend class reply;
    };


    /*! \brief A reply from a service containing the result of requested work.
     */
    class reply
    {
        detail::header head;
        optional_part client_;
        part          client_delimiter;
        many_parts    metadata_;
        part          metadata_delimiter;
        many_parts    data_;

        reply(detail::header && head,
              optional_part && client_,
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

            auto client_            = read_optional(iter, end);
            auto client_delimiter   = read_part(iter, end);
            auto metadata           = read_many(iter, end);
            auto metadata_delimiter = read_part(iter, end);
            auto data               = read_many(iter, end);

            return reply(std::move(head),
                         std::move(client_),
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

        auto inline address() const noexcept -> boost::optional<msg::address> {
            return head.address();
        }

        auto inline address(msg::address const & addr) -> void {
            head.address(addr);
        }

        auto inline client() const noexcept -> boost::optional<msg::address> {
            if (client_) {
                return std::string(client_->data<char>(), client_->size());
            } else {
                return boost::none;
            }
        }

        auto inline client(msg::address const & addr) noexcept -> void {
            client_ = part(addr.data(), addr.size());
        }

        friend auto read(std::vector<zmq::message_t> && parts) -> any_message;
        friend class detail::sender;
    };

    /*! \brief A re-connect message signalling a worker to re-register.
     *
     * When the broker receives a ping from a worker that is not
     * registered with it, this can mean that the worker expects that
     * it is registered. In that case, the broker will reply with a
     * reconnect message to signal the worker that it is not actually
     * registered.
     */
    class reconnect
    {
        detail::header head;

        reconnect(detail::header && head);

        auto send(detail::part_sink & sink) -> void;

        template <class iterator>
        auto static read(detail::header && h, iterator &, iterator &)
            -> reconnect {
            return reconnect(std::move(h));
        }

        enum detail::types static const type = detail::types::reconnect;
    public:
        /*! \brief Create a message signalling a worker to re-register.
         */
        auto static make(ping && p) noexcept -> reconnect;

        friend auto read(std::vector<zmq::message_t> && parts) -> any_message;
        friend class detail::sender;
    };
};
