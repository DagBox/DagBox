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
#include "lock.hpp"


auto lock::lock::operator()(msg::request && request) -> std::vector<zmq::message_t>
{
    for (auto & data : request.data()) {
        msgpack::object_handle req_obj = msgpack::unpack(data.data<char>(), data.size());
        auto req = req_obj.get().as<detail::lock_request>();
        bool status;

        if (req.lock) {
            auto result = locks.insert(req.key);
            status = result.second;
        } else {
            auto result = locks.erase(req.key);
            status = result > 0;
        }

        msgpack::sbuffer buffer;
        msgpack::pack(buffer, status);
        data.rebuild(buffer.data(), buffer.size());
    }

    return msg::send(msg::reply::make(std::move(request)));
}
