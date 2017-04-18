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
#include "datastore.hpp"
#include "../msgpack_boost_flatmap.hpp"
#include <boost/container/flat_map.hpp>
#include <boost/uuid/uuid.hpp>
#include <msgpack/adaptor/boost/optional.hpp>
using namespace data;

namespace container=boost::container;
namespace uuid=boost::uuids;


storage::storage(filesystem::path const & directory)
    : env(lmdb::env::create())
{
    set_max_dbs(max_buckets);
    open(directory.c_str());
}





datastore::datastore(storage & env)
    : env(env)
{}


auto datastore::get_open_bucket(std::string bucket_name, lmdb::txn & txn)
    -> lmdb::dbi &
{
    try {
        return buckets.at(bucket_name);
    } catch (std::out_of_range) {
        auto result = buckets.emplace(
            bucket_name,
            lmdb::dbi::open(
                txn,
                bucket_name.c_str(),
                bucket_open_flags()));
        return result.first->second; // first is the iterator, iterator's second is the dbi
    }
}


auto datastore::operator()(msg::request && request) -> std::vector<zmq::message_t>
{
    auto txn = lmdb::txn::begin(env);
    for (auto & data : request.data()) {
        msgpack::object_handle req_obj = msgpack::unpack(data.data<char>(), data.size());
        auto buffer = process_request(req_obj, txn);
        // TODO: Create a subclass of zmq::message_t that can be used
        // as a buffer for msgpack
        data.rebuild(buffer.data(), buffer.size());
    }
    txn.commit();
    return msg::send(msg::reply::make(std::move(request)));
}




struct read_request
{
    std::string bucket;
    std::string key;
    boost::optional<std::string> data;

    // Boost map that allows incomplete types
    container::flat_map<std::string, read_request> relations;

    MSGPACK_DEFINE_MAP(bucket, key, data, relations);
};


auto reader::process_request(msgpack::object_handle & req, lmdb::txn & txn)
    -> msgpack::sbuffer
{
    // An anonymous function, hidden here to avoid declaring it inside
    // the header while still writing it recursively
    std::function<void(read_request &)> process;
    process = [&](read_request & request) {
        auto & bucket = get_open_bucket(request.bucket, txn);
        auto status = bucket.get(txn, request.key, request.data);
        assert(status); // TODO: Throw an exception if we can't find the key
        for (auto & rel : request.relations) {
            process(rel.second);
        }
    };

    auto request = req.get().as<read_request>();
    process(request);
    msgpack::sbuffer buffer;
    msgpack::pack(buffer, request);
    return buffer;
}


auto reader::bucket_open_flags() const -> unsigned int { return lmdb::dbi::default_flags; }
auto reader::txn_begin_flags() const -> unsigned int { return MDB_RDONLY; }




struct write_request
{
    std::string bucket;
    std::string data;

    MSGPACK_DEFINE_MAP(bucket, data);
};


auto writer::process_request(msgpack::object_handle & req, lmdb::txn & txn)
    -> msgpack::sbuffer
{
    auto request = req.get().as<write_request>();
    auto & bucket = get_open_bucket(request.bucket, txn);
    auto key = key_generator();
    bucket.put(txn, key.data, request.data);

    msgpack::sbuffer buffer;
    msgpack::pack(buffer, key.data);
    return buffer;
}


auto writer::bucket_open_flags() const -> unsigned int { return MDB_CREATE; }
auto writer::txn_begin_flags() const -> unsigned int { return lmdb::txn::default_flags; }
