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
#include <boost/container/flat_map.hpp>
#include <zmq.hpp>
#include <msgpack.hpp>
#include "../msgpack_boost_flatmap.hpp"
#include <msgpack/adaptor/boost/optional.hpp>
#include <lmdb++.h>
#include "../message.hpp"

namespace filesystem = boost::filesystem;

/*! \file datastore.hpp
 * Key-value storage worker and related utilities.
 */



/*! \brief Key-value storage worker and related utilities.
 */
namespace data
{
    namespace detail
    {
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

        struct write_request
        {
            std::string bucket;
            std::string data;

            MSGPACK_DEFINE_MAP(bucket, data);
        };
    };

    /*! \brief An LMDB storage environment.
     *
     * This class is a
     * [lmdb::env](http://lmdbxx.sourceforge.net/classlmdb_1_1env.html),
     * which opens itself automatically when created.
     */
    class storage : public lmdb::env
    {
    public:
        /*! \brief Maximum number of buckets that can be opened. */
        const static uint_fast8_t max_buckets = 32;
        /*! \brief Create a data storage.
         *
         * \param directory The directory where data will be stored.
         * Will be automatically created if it is missing. The program
         * must have write permissions in the given directory.
         */
        storage(filesystem::path const & directory);
    };


    /*! \brief The base class for all key-value storage workers.
     *
     * Provides various utilities for accessing key-value
     * [storage](\ref storage). Requests and replies are expected to
     * be serialized with messagepack. Child classes must override the
     * protected functions provided here to implement their own
     * functionality.
     *
     * This class also implements the required interface for
     * [assistant](\ref assistant), thus the classes that inherit it
     * will be compatible with assistant.
     */
    class datastore
    {
        storage & env;
        std::unordered_map<std::string, lmdb::dbi> buckets;
    protected:
        /*! \brief Get a bucket.
         *
         * If the bucket doesn't exist, it will be created. If it does
         * exist but it hasn't been opened yet, it will be
         * opened. In any case, the existing open bucket will be returned.
         *
         * \param bucket_name The name of the requested bucket. Any
         * string may be used, subject to limitations of LMDB.
         * \param txn The current transaction.
         */
        auto get_open_bucket(std::string bucket_name, lmdb::txn & txn)
            -> lmdb::dbi &;
        /*! \brief Process a request and return a response.
         *
         * Child classes must override this method to provide their
         * own request processing behaviour. The datastore will handle
         * details such as de-serializing of the request and opening
         * an LMDB transaction.
         *
         * \param req The request that needs to be processed.
         * \param txn The current transaction that has been opened for
         * this request.
         */
        auto virtual process_request(msgpack::object_handle & req,
                                     lmdb::txn & txn)
            -> msgpack::sbuffer = 0;
        /*! \brief The flags that should be used when opening buckets.
         *
         * The datastore will open LMDB databases using the flags
         * returned by this function.  See the flags in [LMDB's
         * documentation](http://www.lmdb.tech/doc/group__mdb.html#gac08cad5b096925642ca359a6d6f0562a)
         * for possible values this function may return.
         */
        auto virtual bucket_open_flags() const -> unsigned int = 0;
        /*! \brief The flags that should be used when starting transactions .
         *
         * The datastore will start LMDB transactions using the flags
         * returned by this function.  See the flags in [LMDB's
         * documentation](http://www.lmdb.tech/doc/group__mdb.html#gad7ea55da06b77513609efebd44b26920)
         * for possible values this function may return.
         */
        auto virtual txn_begin_flags() const -> unsigned int = 0;
    public:
        /*! \brief The name of the service.
        *
        * Inheriting classes may override this name with a name of
        * their choice.
        */
        std::string const service_name = "datastore";
        /*! \brief Create a datastore worker.
         *
         * \param env Storage that the datastore worker will use.
         */
        datastore(storage & env);
        /*! \brief Process a data storage request.
         *
         * \param request A request message for data storage.
         *
         * \returns A reply message that contains the response to the
         * requst, ready to be sent.
         */
        auto operator()(msg::request && request) -> std::vector<zmq::message_t>;
    };


    /*! \brief A datatore reader.
     */
    class reader : public datastore
    {
        auto process_request(msgpack::object_handle & req, lmdb::txn & txn)
            -> msgpack::sbuffer override;
        auto bucket_open_flags() const -> unsigned int override;
        auto txn_begin_flags() const -> unsigned int override;
    public:
        std::string const service_name = "datastore reader";
        using datastore::datastore;
    };


    /*! \brief A datatore writer.
     */
    class writer : public datastore
    {
        boost::uuids::basic_random_generator<boost::mt19937> key_generator;

        auto process_request(msgpack::object_handle & req, lmdb::txn & txn)
            -> msgpack::sbuffer override;
        auto bucket_open_flags() const -> unsigned int override;
        auto txn_begin_flags() const -> unsigned int override;
    public:
        std::string const service_name = "datastore writer";
        using datastore::datastore;
    };
};
