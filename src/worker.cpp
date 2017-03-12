#include "worker.hpp"


worker::worker(zmq::context_t & ctx, std::string const & broker_addr)
    : sock(ctx, socket_type)
{
    // Timeout on recv so we can stop waiting and send a ping to the
    // broker
    sock.setsockopt(ZMQ_RCVTIMEO, recv_timeout_ms);
    sock.connect(broker_addr);
}


template <class runner>
auto worker::run(zmq::context_t & ctx, std::string const & broker_addr) -> void
{
    // The worker base which holds the socket
    worker work(ctx, broker_addr);
    // The runner object which does the actual work
    runner run;

    while (true) {
        auto message = work.sock.recv_multimsg();
        if (message.size() > 0) { // If the recv didn't time out
            auto maybe_reply = boost::apply_visitor(run, message);
            if (maybe_reply) {
                work.sock.send_multimsg(*maybe_reply);
            }
        } else {
            // Check if the broker is still alive
            work.sock.send_multimsg(msg::ping::make());
        }
    }
}
