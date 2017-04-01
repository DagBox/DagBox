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
#include <msgpack/adaptor/boost/optional.hpp>
using namespace datastore;

namespace container=boost::container;


storage::storage(filesystem::path const & directory)
    : env(lmdb::env::create())
{
    open(directory.c_str());
}


auto open_bucket(datastore::storage & env, const std::string & bucket_name)
    -> lmdb::dbi
{
    auto txn = lmdb::txn::begin(env);
    auto dbi = lmdb::dbi::open(txn, bucket_name.c_str());
    txn.commit();
    return dbi;
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


reader::reader(storage & env)
    : env(env)
{}


auto reader::get_open_bucket(std::string bucket_name) -> lmdb::dbi &
{
    try {
        return buckets.at(bucket_name);
    } catch (std::out_of_range) {
        auto result = buckets.emplace(bucket_name, open_bucket(env, bucket_name));
        return result.first->second; // first is the iterator, iterator's second is the dbi
    }
}


auto reader::operator()(msg::request && request) -> std::vector<zmq::message_t>
{
    for (auto & data : request.data()) {
        auto req_obj = msgpack::unpack(data.data<char>(), data.size());
    }
}




writer::writer(storage & env, std::string const & bucket_name)
    : env(env),
      bucket(open_bucket(env, bucket_name)),
      service_name("datastore writer <" + bucket_name + ">")
{}


auto writer::operator()(msg::request && request) -> std::vector<zmq::message_t>
{

}
