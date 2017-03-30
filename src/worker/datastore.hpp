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

#include <vector>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <zmq.hpp>
#include <msgpack.hpp>
#include <lmdb++.h>
#include "../message.hpp"

namespace filesystem = boost::filesystem;


namespace datastore
{
    /*! \brief An LMDB storage environment.
     *
     * This class is a
     * [lmdb::env](http://lmdbxx.sourceforge.net/classlmdb_1_1env.html),
     * which opens itself automatically when created.
     */
    class storage : public lmdb::env
    {
    public:
        storage(filesystem::path const & directory);
    };


    class reader
    {
        storage & env;
    public:
        std::string const service_name = "datastore reader";
        reader(storage & env);
        auto operator()(msg::request && request) -> std::vector<zmq::message_t>;
    };


    class writer
    {
        storage & env;
        lmdb::dbi dbi;
    public:
        std::string const service_name;
        writer(storage & env, std::string const & bucket_name);
        auto operator()(msg::request && request) -> std::vector<zmq::message_t>;
    };
};
