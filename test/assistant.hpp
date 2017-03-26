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



// A worker that echoes the requests.
struct test_worker_echo
{
    std::string const service_name = "test worker echo";
    auto operator()(msg::request && req) -> std::vector<zmq::message_t> {
        return msg::send(req);
    }
};


auto test_assistant() -> void {
    describe("assistant", [](){
        zmq::context_t ctx;
        std::string addr = "inproc://test_assistant";
        socket sock(ctx, zmq::socket_type::router);
        sock.setsockopt(ZMQ_RCVTIMEO, 4000); // in ms
        sock.bind(addr);

        assistant<test_worker_echo> echo_worker(ctx, addr, 500);

        it("registers itself when created", [&](){
            auto msg = msg::read(sock.recv_multimsg());
            auto reg = std::move(boost::get<msg::registration>(msg));
            sock.send_multimsg(msg::send(reg));
        });
    });
};
