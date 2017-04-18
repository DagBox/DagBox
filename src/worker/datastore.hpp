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
#include <boost/uuid/random_generator.hpp>
#include <boost/filesystem.hpp>
#include <zmq.hpp>
#include <msgpack.hpp>
#include <lmdb++.h>
#include "../message.hpp"

namespace filesystem = boost::filesystem;


namespace data
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
        const static uint_fast8_t max_buckets = 32;
        storage(filesystem::path const & directory);
    };


    /*! \brief The base class for all data storing components.
     */
    class datastore
    {
        storage & env;
    protected:
        std::unordered_map<std::string, lmdb::dbi> buckets;

        auto get_open_bucket(std::string bucket_name, lmdb::txn & txn)
            -> lmdb::dbi &;
        auto virtual process_request(msgpack::object_handle & req,
                                     lmdb::txn & txn)
            -> msgpack::sbuffer = 0;
    public:
        std::string const service_name = "datastore reader";
        datastore(storage & env);
        auto operator()(msg::request && request) -> std::vector<zmq::message_t>;
    };


    /*! \brief A datatore reader.
     */
    class reader : public datastore
    {
        auto process_request(msgpack::object_handle & req, lmdb::txn & txn)
            -> msgpack::sbuffer override;
    public:
        using datastore::datastore;
    };


    /*! \brief A datatore writer.
     */
    class writer : public datastore
    {
        boost::uuids::basic_random_generator<boost::mt19937> key_generator;

        auto process_request(msgpack::object_handle & req, lmdb::txn & txn)
            -> msgpack::sbuffer override;
    public:
        using datastore::datastore;
    };
};
