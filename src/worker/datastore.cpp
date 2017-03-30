#include "datastore.hpp"
using namespace datastore;


storage::storage(filesystem::path const & directory)
    : env(lmdb::env::create())
{
    open(directory.c_str());
}


reader::reader(storage & env)
    : env(env)
{}


auto open_bucket(datastore::storage & env, const std::string & bucket_name)
    -> lmdb::dbi
{
    auto txn = lmdb::txn::begin(env);
    auto dbi = lmdb::dbi::open(txn, bucket_name.c_str());
    txn.commit();
    return dbi;
}


writer::writer(storage & env, std::string const & bucket_name)
    : env(env),
      dbi(open_bucket(env, bucket_name)),
      service_name("datastore writer <" + bucket_name + ">")
{}
