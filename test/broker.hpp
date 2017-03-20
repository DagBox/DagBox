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
#pragma once

#include <memory>
#include <zmq.hpp>
#include "helpers.hpp"
#include "../src/broker.hpp"


auto test_broker = [](){
    describe("broker", [](){
        zmq::context_t ctx;
        std::string br_addr = "inproc://test";
        component<broker> broker_component(ctx, br_addr, std::chrono::milliseconds{1000});

        socket sock(ctx, zmq::socket_type::dealer);
        // Setting a timeout on the socket so that the tests don't
        // hang indefinitely
        sock.setsockopt(ZMQ_RCVTIMEO, 500); // in ms
        sock.connect(br_addr);

        it("can register workers", [&](){
            sock.send_multimsg(msg::send(msg::registration::make("test_service")));
            auto rep = msg::read(sock.recv_multimsg());
            boost::get<msg::registration>(rep);
        });

        it("can respond to pings", [&](){
            sock.send_multimsg(msg::send(msg::ping::make()));
            auto rep = msg::read(sock.recv_multimsg());
            boost::get<msg::pong>(rep);
        });

        it("can handle requests and replies", [&](){
            sock.send_multimsg(msg::send(msg::request::make("test_service",
                                                            msg_vec({"meta"}),
                                                            msg_vec({"data", "more data"}))));
            auto req = msg::read(sock.recv_multimsg());
            auto & sent_request = boost::get<msg::request>(req);

            sock.send_multimsg(msg::send(msg::reply::make(std::move(sent_request))));
            auto rep = msg::read(sock.recv_multimsg());
            boost::get<msg::reply>(rep);
        });
    });
};
