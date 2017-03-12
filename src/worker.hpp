#include <string>
#include <boost/variant.hpp>
#include <zmq.hpp>
#include "message.hpp"
#include "socket.hpp"
#include "helpers.hpp"


class worker
{
    auto const static socket_type = zmq::socket_type::dealer;
    int const static recv_timeout_ms = 500;

    socket sock;
    worker(zmq::context_t & ctx, std::string const & broker_addr);


public:
    template <class runner>
    auto static run(zmq::context_t & ctx, std::string const & broker_addr)
        -> void;
};
