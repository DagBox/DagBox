#include "helpers.hpp"
#include "../src/message.hpp"


TEST([](){
    describe("message part readers", [&](){
        it("can read message parts", [&](){
            auto msgs = msg_vec({"first", "second"});

            auto b = msgs.begin();
            auto e = msgs.end();

            auto p = msg::read_part(b, e);

            AssertThat(msg2str(p), Equals("first"));
        });
    });
});
