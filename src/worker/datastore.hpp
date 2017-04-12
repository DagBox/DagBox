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
#include "../msgpack_boost_flatmap.hpp"
#include <boost/container/flat_map.hpp>
#include <msgpack/adaptor/boost/optional.hpp>
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


    namespace detail {
        namespace container=boost::container;

        struct read_request
        {
            std::string bucket;
            std::string key;
            boost::optional<std::string> data;

            // Boost map that allows incomplete types
            container::flat_map<std::string, read_request> relations;

            MSGPACK_DEFINE_MAP(bucket, key, data, relations);
        };
    };

    /*! \brief A reader that can read from any bucket.
     */
    class reader
    {
        storage & env;
        std::unordered_map<std::string, lmdb::dbi> buckets;

        auto get_open_bucket(std::string bucket_name, lmdb::txn & txn)
            -> lmdb::dbi &;
        auto fill_read_request(detail::read_request & req,
                               lmdb::txn & txn) -> void;
    public:
        std::string const service_name = "datastore reader";
        reader(storage & env);
        auto operator()(msg::request && request) -> std::vector<zmq::message_t>;
    };


    /*! \brief A datatore writer that writes to a single bucket.
     */
    class writer
    {
        storage & env;
        lmdb::dbi bucket;
        auto static open(storage & env,
                         std::string const & bucket_name)
            -> lmdb::dbi;
    public:
        std::string const service_name;
        writer(storage & env, std::string const & bucket_name);
        auto operator()(msg::request && request) -> std::vector<zmq::message_t>;
    };
};
