#include "datastore.hpp"
using namespace datastore;


auto detail::storage::get_env() -> lmdb::env &
{
    if (!env_maybe) {
        std::lock_guard<std::mutex> lock(init);

        // While we were waiting for the lock, someone else might have
        // initialized the env
        if (!env_maybe) {
            env_maybe = lmdb::env::create();
        }
    }
    return *env_maybe;
}
