#include <iostream>
#include <sstream>
#include <cstddef>
using std::size_t;

#include <lmdb.h>

#include <zmq.hpp>
#include "socket.hpp"
#include "message.hpp"

#include <msgpack.hpp>
#include <msgpack/sbuffer.hpp>

#include <boost/coroutine2/coroutine.hpp>
typedef boost::coroutines2::coroutine<zmq::message_t> message_stream;


auto main(void) -> int
{
    zmq::context_t context;
    class socket alice(context, zmq::socket_type::pair);
    class socket bob(context, zmq::socket_type::pair);

    alice.bind("inproc://alice");
    bob.connect("inproc://alice");

    message_stream::pull_type source(
        [&](message_stream::push_type & sink) {
            msgpack::sbuffer buffer;
            msgpack::type::tuple<int, bool, std::string> object(1, true, "Hello");
            msgpack::pack(buffer, object);
            zmq::message_t msg(buffer.data(), buffer.size());
            sink(msg);


            std::string s;

            s = "first";
            zmq::message_t hello;

            hello = zmq::message_t(s.data(), s.size());
            sink(hello);

            s = "second";
            hello = zmq::message_t(s.data(), s.size());
            sink(hello);

            s = "third";
            hello = zmq::message_t(s.data(), s.size());
            sink(hello);
        });
    alice.send_multimsg<message_stream::pull_type::iterator>(begin(source), end(source));

    {
        auto recv_stream = bob.recv_multimsg();

        auto & msg = recv_stream[0];
        msgpack::object_handle object_handle = msgpack::unpack((char*)msg.data(), msg.size());
        auto object = object_handle.get();
        std::cout << object << std::endl;

        for (auto & s : recv_stream) {
            std::cout << std::string((char*)s.data()) << std::endl;
        }
    }

    alice.close();
    bob.close();

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
