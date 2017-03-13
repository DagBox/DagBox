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
