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

#include "helpers.hpp"
#include "../src/assistant.hpp"



#define TEST_WORKER_ECHO_OP(msg_class)                  \
    auto operator()(msg_class & msg)                    \
        -> boost::optional<std::vector<zmq::message_t>> \
    {                                                   \
        return msg::send(std::move(msg));               \
    }



// A worker that echoes whatever message given.
struct test_worker_echo
    : boost::static_visitor<boost::optional<std::vector<zmq::message_t>>>
{
    TEST_WORKER_ECHO_OP(msg::ping);
    TEST_WORKER_ECHO_OP(msg::pong);
    TEST_WORKER_ECHO_OP(msg::registration);
    TEST_WORKER_ECHO_OP(msg::request);
    TEST_WORKER_ECHO_OP(msg::reply);
    TEST_WORKER_ECHO_OP(msg::reconnect);
};


auto test_assistant() -> void {
    describe("assistant", [](){
        zmq::context_t ctx;
        std::string addr = "inproc://test_assistant";
        socket sock(ctx, zmq::socket_type::router);
        sock.setsockopt(ZMQ_RCVTIMEO, 4000); // in ms
        sock.bind(addr);

        assistant<test_worker_echo> echo_worker(ctx, addr, 500);

        it("sends ping if it hasn't received anything", [&](){
            // We haven't sent anything, the assistant should send a ping
            // after waiting for some time
            echo_worker.run();
            auto msg = msg::read(sock.recv_multimsg());
            boost::get<msg::ping>(msg);
        });
        it("passes messages to the worker", [&](){
            sock.send_multimsg(msg::send(msg::registration::make("test service")));
            echo_worker.run();
            auto msg = msg::read(sock.recv_multimsg());
            boost::get<msg::registration>(msg);
        });
    });
};
