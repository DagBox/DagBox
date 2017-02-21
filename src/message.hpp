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
namespace message
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
    enum class type
    {
        registration = 0x01,
        ping = 0x02,
        pong = 0x03,
        request = 0x04,
        reply = 0x05,
    };
    namespace detail
    {
        auto const type_upper_bound = static_cast<char>(type::reply);
        auto const type_lower_bound = static_cast<char>(type::registration);
    }


    /*! \brief The header parts of the message which is common in all
     * message types.
     *
     */
    class header {
        auto static make_protocol_part() -> zmq::message_t;
        auto static make_type_part(enum type type_) -> zmq::message_t;

        /*! Validate the header object, throw an exception if it is invalid. */
        auto validate_or_throw() -> void;

        /*! Move the message parts into the object.
         */
        header(zmq::message_t && init_part,
               zmq::message_t && protocol_part,
               zmq::message_t && type_part);
    protected:
        zmq::message_t init_part;
        zmq::message_t protocol_part;
        zmq::message_t type_part;
    public:
        /*! \brief Create a new message header with the given type. */
        auto static make(enum type type_) -> header;

        /*! \brief Read a message header from a collection of messages.
         *
         * ```
         * header h(socket.recv_multimsg());
         * ```
         *
         * \returns The header object.
         *
         * \throws exception::malformed The message is not in the
         * DagBox protocol format.
         *
         * \throws exception::unsupported_version The message comes
         * from an unsupported version of the DagBox protocol.
         */
        auto static make(std::vector<zmq::message_t> && messages) -> header;

        /*! \brief Get the type of the message.
         *
         * If the recieved message has an invalid type, this will
         * throw a [exception::malformed][\ref exception::malformed]
         * exception.
         */
        auto type() const -> enum type;

        /*! \brief Change the type of the message. */
        auto type(enum type new_type) noexcept -> void;
    };

    // Specialized message types
    class registration;
    class ping;
    class pong;
    class request;
    class reply;


    /*! \brief Builds a message from a collection of message parts.
     */
    auto read(std::vector<zmq::message_t> && messages)
        -> boost::variant<registration,
                          ping,
                          pong,
                          request,
                          reply>;


    class registration : public header {
    protected:
        zmq::message_t service_part;

        registration(zmq::message_t && init_part,
                     zmq::message_t && protocol_part,
                     zmq::message_t && type_part,
                     zmq::message_t && service_part);
    public:
        /*! Build a registration message that registers for
         *  `service_name`.
         */
        auto static make(std::string service_name) -> registration;

        /*! Build a registration message from a read message.
         */
        auto static make(header && h) -> registration;
    };

    class ping : public header {
    public:
        auto static make() -> ping;

        auto static make(header && h) -> ping;
    };

    class pong : public header {
    public:
        auto static make(ping && p) -> pong;

        auto static make(header && h) -> pong;
    };

    class request : public header {
    protected:
        zmq::message_t service_part;
        boost::optional<zmq::message_t> client_part;
        zmq::message_t client_delimiter_part;
        std::vector<zmq::message_t> metadata_parts;
        zmq::message_t metadata_delimiter_part;
        std::vector<zmq::message_t> data_parts;

        request(zmq::message_t && init_part,
                zmq::message_t && protocol_part,
                zmq::message_t && type_part,
                zmq::message_t && service_part,
                boost::optional<zmq::message_t> && client_part,
                zmq::message_t && client_delimiter_part,
                std::vector<zmq::message_t> && metadata_parts,
                zmq::message_t && metadata_delimiter_part,
                std::vector<zmq::message_t> && data_parts);
    public:
        auto static make(std::string service,
                         std::vector<zmq::message_t> && metadata,
                         std::vector<zmq::message_t> && data)
            -> request;

        auto static make(header && h) -> request;
    };

    class reply : public header {
        boost::optional<zmq::message_t> client_part;
        zmq::message_t client_delimiter_part;
        std::vector<zmq::message_t> metadata_parts;
        zmq::message_t metadata_delimiter_part;
        std::vector<zmq::message_t> data_parts;

        reply(zmq::message_t && init_part,
              zmq::message_t && protocol_part,
              zmq::message_t && type_part,
              boost::optional<zmq::message_t> && client_part,
              zmq::message_t && client_delimiter_part,
              std::vector<zmq::message_t> && metadata_parts,
              zmq::message_t && metadata_delimiter_part,
              std::vector<zmq::message_t> && data_parts);
    public:
        auto static make(request && r) -> reply;

        auto static make(header && h) -> reply;
    };

};
