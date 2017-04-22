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
#include <vector>
#include <initializer_list>

#include <bandit/bandit.h>
using namespace bandit;
using namespace snowhouse;

#include <zmq.hpp>
#include <msgpack.hpp>



auto msg2str(zmq::message_t const & msg) -> std::string
{
    return std::string(msg.data<char>(), msg.size());
}


auto msg_vec(std::initializer_list<std::string> msgs)
    -> std::vector<zmq::message_t>
{
    std::vector<zmq::message_t> vec;
    for (auto msg : msgs) {
        if (msg.size() == 0) {
            vec.push_back(zmq::message_t());
        } else {
            vec.push_back(zmq::message_t(msg.data(), msg.size()));
        }
    }
    return vec;
}


// Turn an object to a string using msgpack
template <class T>
auto dumps(T t) -> std::string
{
    std::stringstream s;
    msgpack::pack(s, t);
    return s.str();
}


template <class T, class S>
auto loads(S & str_like) -> T
{
    msgpack::object_handle h = msgpack::unpack(
        static_cast<char *>(str_like.data()),
        str_like.size());
    return h.get().as<T>();
}
