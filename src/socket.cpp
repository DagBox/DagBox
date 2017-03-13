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
#include "socket.hpp"


auto socket::recv_multimsg() -> std::vector<zmq::message_t>
{
    std::vector<zmq::message_t> messages;
    bool has_more = true;
    while (has_more) {
        zmq::message_t message;
        auto recv_size = recv(&message);
        if (recv_size == 0) { // recv timed out
            // recv should only timeout if we recieved no message at all
            assert(messages.size() == 0);
            return messages;
        }
        has_more = message.more();
        messages.push_back(std::move(message));
    }
    return messages;
}
