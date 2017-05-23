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
#include "client.hpp"
using namespace dgbx;

#include <sstream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>


auto transport_name(enum transport transport) noexcept -> std::string
{
    switch(transport) {
    case transport::inprocess: return "inproc";
    case transport::ipc: return "ipc";
    case transport::tcp: return "tcp";
    }
}


auto form_address(enum transport transport, std::string broker_address)
    -> std::string
{
    std::stringstream s;
    // Address is in form of "transport://address"
    s << transport_name(transport) << "://";
    // Generate a unique address if none is given
    if (broker_address == "") {
        boost::uuids::basic_random_generator<boost::mt19937> static keygen;
        s << keygen();
    } else {
        s << broker_address;
    }
    return s.str();
}


dagbox::dagbox(
    filesystem::path data_directory,
    enum transport transport,
    std::string broker_address,
    uint reader_count,
    std::chrono::milliseconds worker_timeout,
    std::chrono::milliseconds transport_delay)
    : address(form_address(transport, broker_address)),
      worker_timeout(worker_timeout),
      storage(data_directory),
      broker(context, address, worker_timeout + transport_delay),
      writer(context, address, worker_timeout.count(), std::ref(storage))
{
    reader_add(reader_count);
}


auto dagbox::reader_add(uint count) -> void
{
    while (count > 0) {
        readers.push_back(component<assistant<data::reader>>(
                              context,
                              address,
                              worker_timeout.count(),
                              std::ref(storage)));
        --count;
    }
}


auto dagbox::reader_remove(uint count) -> void
{
    while (count > 0 && reader_count() > 0) {
        readers.pop_back();
        --count;
    }
}


auto dagbox::reader_count() const -> uint
{
    return readers.size();
}
