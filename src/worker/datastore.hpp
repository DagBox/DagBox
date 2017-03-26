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

#include <mutex>
#include <vector>
#include <boost/optional.hpp>
#include <zmq.hpp>
#include <msgpack.hpp>
#include <lmdb++.h>
#include "../message.hpp"


namespace datastore
{
    namespace detail
    {
        class storage
        {
            std::mutex static init;
            boost::optional<lmdb::env> static env_maybe;
            auto static get_env() -> lmdb::env &;
        protected:
            lmdb::env & env = get_env();
        };
    };


    class reader : private detail::storage
    {
    public:
        std::string const service_name = "datastore reader";
        reader();
        auto operator()(msg::request && request) -> std::vector<zmq::message_t>;
    };


    class writer : private detail::storage
    {
    public:
        std::string const service_name;
        writer(std::string const & bucket_name);
        auto operator()(msg::request && request) -> std::vector<zmq::message_t>;
    };
};
