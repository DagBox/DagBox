#pragma omp once

#include <vector>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include "socket.hpp"

/*! \file message.hpp
 * Helpers for handling messages.
 */



/*! \brief Used to declare an exception class.
 *
 * Declares an exception class with the name
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

        /*! \brief A message with an invalid type was recieved.
         *
         * This is not an exception that is normally
         * expected. Recieveing this message suggests that the sender
         * is broken.
         */
        EXCEPTION(invalid_type, runtime_error);
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


    /*! \brief The header parts of the message which is common in all
     * message types.
     *
     */
    class header {
        auto static make_protocol_part() -> zmq::message_t;
        auto static make_type_part(enum type type_) -> zmq::message_t;
    protected:
        zmq::message_t init_part;
        zmq::message_t protocol_part;
        zmq::message_t type_part;
    public:
        /*! \brief Create a new message header with the given type. */
        header(enum type type_);

        /*! \brief Read a message header from a stream of message
         * parts.
         *
         * This constructor accepts anything that can be iterated. The
         * iterator should return message parts with type
         * `zmq::message_t`. A suitable iterator can be created using
         * [socket.recv_multimsg()][\ref socket.recv_multimsg()].
         */
        template <class iterator>
        header(iterator it);

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


    /*! \brief Builds a message from a stream of message parts.
     */
    auto make(message_stream::pull_type parts)
        -> boost::variant<registration,
                          ping,
                          pong,
                          request,
                          reply>;


    class registration : public header {
        zmq::message_t service_part;
    public:
        enum type const type = type::registration;
    };

    class ping : public header {
    public:
        enum type const type = type::ping;
    };

    class pong : public header {
    public:
        enum type const type = type::pong;
    };

    class request : public header {
        zmq::message_t service_part;
        boost::optional<zmq::message_t> client_part;
        zmq::message_t client_delimiter_part;
        std::vector<zmq::message_t> metadata_parts;
        zmq::message_t metadata_delimiter_part;
        std::vector<zmq::message_t> data_parts;
    public:
        enum type const type = type::request;
    };

    class reply : public header {
        boost::optional<zmq::message_t> client_part;
        zmq::message_t client_delimiter_part;
        std::vector<zmq::message_t> metadata_parts;
        zmq::message_t metadata_delimiter_part;
        std::vector<zmq::message_t> data_parts;
    public:
        enum type const type = type::reply;
    };

};
