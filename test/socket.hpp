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

#include "helpers.hpp"
#include "../src/socket.hpp"



auto test_socket = [](){
    zmq::context_t context;

    describe("socket", [&](){
        class socket server(context, zmq::socket_type::pair);
        class socket client(context, zmq::socket_type::pair);
        server.bind("inproc://test-socket");
        client.connect("inproc://test-socket");

        it("sends and recieves multi-part messages", [&](){
            auto msgs = msg_vec({"first", "", "last"});
            client.send_multimsg(std::move(msgs));
            auto recv_msgs = server.recv_multimsg();

            AssertThat(msg2str(recv_msgs[0]), Equals("first"));
            AssertThat(recv_msgs[1].size(), Equals<uint>(0));
            AssertThat(msg2str(recv_msgs[2]), Equals("last"));
        });
    });
};
