/*
  Copyright 2017 Kaan Genç

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
