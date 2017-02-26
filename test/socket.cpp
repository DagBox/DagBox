#include "helpers.hpp"
#include "../src/socket.hpp"



go_bandit([](){
    zmq::context_t context;

    describe("socket", [&](){
        socket server(context, zmq::socket_type::pair);
        socket client(context, zmq::socket_type::pair);
        server.bind("inproc://test-socket");
        client.connect("inproc://test-socket");

        it("sends and recieves multi-part messages", [&](){
            auto msgs = msg_vec({"first", "", "last"});
            client.send_multimsg(msgs.begin(), msgs.end());
            auto recv_msgs = server.recv_multimsg();

            AssertThat(msg2str(recv_msgs[0]), Equals("first"));
            AssertThat(recv_msgs[1].size(), Equals(0));
            AssertThat(msg2str(recv_msgs[2]), Equals("last"));
        });
    });
});


RUN_TEST();
