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
using namespace datastore;

namespace container=boost::container;


storage::storage(filesystem::path const & directory)
    : env(lmdb::env::create())
{
    open(directory.c_str());
}


auto open_bucket(lmdb::txn & txn, const std::string & bucket_name)
    -> lmdb::dbi
{
    auto dbi = lmdb::dbi::open(txn, bucket_name.c_str());
    return dbi;
}




reader::reader(storage & env)
    : env(env)
{}


auto reader::get_open_bucket(std::string bucket_name, lmdb::txn & txn)
    -> lmdb::dbi &
{
    try {
        return buckets.at(bucket_name);
    } catch (std::out_of_range) {
        auto result = buckets.emplace(bucket_name,
                                      open_bucket(txn, bucket_name));
        return result.first->second; // first is the iterator, iterator's second is the dbi
    }
}


auto reader::operator()(msg::request && request) -> std::vector<zmq::message_t>
{
    auto txn = lmdb::txn::begin(env);
    for (auto & data : request.data()) {
        msgpack::object_handle req_obj = msgpack::unpack(data.data<char>(), data.size());
        auto req = req_obj.get().as<detail::read_request>();
        fill_read_request(req, txn);
        msgpack::sbuffer buffer(data.size() * 2);
        msgpack::pack(buffer, req);
        data.rebuild(buffer.data(), buffer.size());
    }
    txn.commit();
}


auto reader::fill_read_request(detail::read_request & req, lmdb::txn & txn)
    -> void
{
    auto & bucket = get_open_bucket(req.bucket, txn);
    auto status = bucket.get(txn, req.key, req.data);
    assert(status); // TODO: Throw an exception if we can't find the key
    for (auto & rel : req.relations) {
        fill_read_request(rel.second, txn);
    }
}




writer::writer(storage & env, std::string const & bucket_name)
    : env(env),
      bucket(open(env, bucket_name)),
      service_name("datastore writer <" + bucket_name + ">")
{}


auto writer::open(storage & env, std::string const & bucket_name)
    -> lmdb::dbi
{
    auto txn = lmdb::txn::begin(env);
    auto bucket = open_bucket(txn, bucket_name);
    txn.commit();
    return bucket;
}


auto writer::operator()(msg::request && request) -> std::vector<zmq::message_t>
{

}
