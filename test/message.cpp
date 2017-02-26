#include "helpers.hpp"
#include "../src/message.hpp"


go_bandit([](){
    describe("message section readers", [](){
        describe("when reading sections", [](){
            it("can read parts", [](){
                auto msgs = msg_vec({"first", "second"});

                auto b = msgs.begin();
                auto e = msgs.end();

                auto p = msg::read_part(b, e);

                AssertThat(msg2str(p), Equals("first"));
            });

            it("throws if there is nothing to read", [](){
                auto msgs = msg_vec({});

                auto b = msgs.begin();
                auto e = msgs.end();

                AssertThrows(msg::exception::malformed, msg::read_part(b, e));
            });
        });

        describe("when reading optional sections", [](){
            it("can read parts", [](){
                auto msgs = msg_vec({"first", "", "last"});

                auto b = msgs.begin();
                auto e = msgs.end();

                auto p = msg::read_optional(b, e);

                AssertThat(msg2str(*p), Equals("first"));
            });

            it("can handle missing parts", [](){
                auto msgs = msg_vec({"", "last"});

                auto b = msgs.begin();
                auto e = msgs.end();

                auto p = msg::read_optional(b, e);

                AssertThat(bool(p), Equals(false));
            });

            it("throws if it isn't actually an optional section", [](){
                // An optional section is either one empty part or a
                // full part followed by an empty part. Otherwise, it
                // should throw an exception.
                auto msgs = msg_vec({"one", "two"});

                auto b = msgs.begin();
                auto e = msgs.end();

                AssertThrows(msg::exception::malformed,
                             msg::read_optional(b, e));
            });
        });

        describe("when reading multi-part sections", [](){
            it("can read multiple parts at the end", [](){
                auto msgs  = msg_vec({"one", "two", "three"});

                auto b = msgs.begin();
                auto e = msgs.end();

                auto ps = msg::read_many(b, e);

                AssertThat(msg2str(ps[0]), Equals("one"));
                AssertThat(msg2str(ps[1]), Equals("two"));
                AssertThat(msg2str(ps[2]), Equals("three"));
            });

            it("can read multiple parts followed by an empty part", [](){
                auto msgs  = msg_vec({"one", "two", "three", "", "last"});

                auto b = msgs.begin();
                auto e = msgs.end();

                auto ps = msg::read_many(b, e);

                AssertThat(msg2str(ps[0]), Equals("one"));
                AssertThat(msg2str(ps[1]), Equals("two"));
                AssertThat(msg2str(ps[2]), Equals("three"));
                AssertThat(ps, HasLength(3));
            });
        });
    });


    describe("ping messages", [](){
        it("can be created", [](){
            auto ping = msg::ping::make();
        });

        it("can be turned into pong messages", [](){
            auto ping = msg::ping::make();
            auto pong = msg::pong::make(std::move(ping));
        });
    });

    describe("request messages", [](){
        it("can be created", [](){
            auto req = msg::request::make("service",
                                          msg_vec({"meta"}),
                                          msg_vec({"data", "more data"}));

            AssertThat(req.metadata(), HasLength(1));
            AssertThat(msg2str(req.metadata()[0]), Equals("meta"));

            AssertThat(req.data(), HasLength(2));
            AssertThat(msg2str(req.data()[0]), Equals("data"));
            AssertThat(msg2str(req.data()[1]), Equals("more data"));
        });

        it("can be turned into reply messages", [](){
            auto req = msg::request::make("service",
                                          msg_vec({"meta"}),
                                          msg_vec({"data"}));
            auto rep = msg::reply::make(std::move(req));

            // Make sure that metadata survives the conversion
            AssertThat(rep.metadata(), HasLength(1));
            AssertThat(msg2str(rep.metadata()[0]), Equals("meta"));
        });
    });
});


RUN_TEST();
