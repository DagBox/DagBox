#include "broker.hpp"


broker::broker(zmq::context_t & ctx, std::string const & addr)
    : sock(ctx, socket_type)
{
    sock.bind(addr);
}


auto broker::run(zmq::context_t & ctx, const std::string & addr) -> void
{
    broker br(ctx, addr);
    // Enter an infinite loop, processing messages
    while (true) {
        auto message = msg::read(br.sock.recv_multimsg());
        msg::message_iterator iter, end;
        std::tie(iter, end) = boost::apply_visitor(br, message);
        br.sock.send_multimsg(iter, end);
    }
}
