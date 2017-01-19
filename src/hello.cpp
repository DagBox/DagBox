#include <iostream>
#include <sstream>
#include <cstddef>
using std::size_t;

#include <lmdb.h>

#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <nanomsg/inproc.h>

#include <msgpack.hpp>


auto main(void) -> int {
        int alice = nn_socket(AF_SP, NN_PAIR);
        int bob = nn_socket(AF_SP, NN_PAIR);
        nn_bind(alice, "inproc://alice");
        nn_connect(bob, "inproc://alice");

        {
                msgpack::type::tuple<int, bool, std::string> object(1, true, "Hello");
                std::stringstream buffer;
                msgpack::pack(buffer, object);
                auto message = buffer.str();
                nn_send(alice, message.data(), message.size(), 0);
        }

        {
                void* message;
                int message_size = nn_recv(bob, &message, NN_MSG, 0);
                msgpack::object_handle object_handle = msgpack::unpack((char*)message, message_size);
                auto object = object_handle.get();
                std::cout << object << std::endl;
                nn_freemsg(message);
        }

        nn_close(alice);
        nn_close(bob);


        MDB_env *env;
        mdb_env_create(&env);
        mdb_env_open(env, "/tmp", MDB_WRITEMAP, 0b110110110);
        MDB_txn *txn;
        mdb_txn_begin(env, NULL, 0, &txn);
        MDB_dbi dbi;
        mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
        mdb_txn_commit(txn);
        mdb_env_close(env);

        return 0;
}
