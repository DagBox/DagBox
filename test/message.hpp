/*
  Copyright 2017 Kaan Genç, Melis Narin Kaya

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
#pragma once

#include "helpers.hpp"
#include "../src/message.hpp"


auto test_message = [](){
    describe("message section readers", [](){
        describe("when reading sections", [](){
            it("can read parts", [](){
                auto msgs = msg_vec({"first", "second"});

                auto b = msgs.begin();
                auto e = msgs.end();

                auto p = msg::detail::read_part(b, e);

                AssertThat(msg2str(p), Equals("first"));
            });

            it("throws if there is nothing to read", [](){
                auto msgs = msg_vec({});

                auto b = msgs.begin();
                auto e = msgs.end();

                AssertThrows(msg::exception::malformed,
                             msg::detail::read_part(b, e));
            });
        });

        describe("when reading optional sections", [](){
            it("can read parts", [](){
                auto msgs = msg_vec({"first", "", "last"});

                auto b = msgs.begin();
                auto e = msgs.end();

                auto p = msg::detail::read_optional(b, e);

                AssertThat(msg2str(*p), Equals("first"));
            });

            it("can handle missing parts", [](){
                auto msgs = msg_vec({"", "last"});

                auto b = msgs.begin();
                auto e = msgs.end();

                auto p = msg::detail::read_optional(b, e);

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
                             msg::detail::read_optional(b, e));
            });
        });

        describe("when reading multi-part sections", [](){
            it("can read multiple parts at the end", [](){
                auto msgs  = msg_vec({"one", "two", "three"});

                auto b = msgs.begin();
                auto e = msgs.end();

                auto ps = msg::detail::read_many(b, e);

                AssertThat(msg2str(ps[0]), Equals("one"));
                AssertThat(msg2str(ps[1]), Equals("two"));
                AssertThat(msg2str(ps[2]), Equals("three"));
            });

            it("can read multiple parts followed by an empty part", [](){
                auto msgs  = msg_vec({"one", "two", "three", "", "last"});

                auto b = msgs.begin();
                auto e = msgs.end();

                auto ps = msg::detail::read_many(b, e);

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

        it("can be sent", [](){
            auto ping = msg::ping::make();
            auto send = msg::send(std::move(ping));

            AssertThat(send, HasLength(3));
            AssertThat(*send[2].data<uint8_t>(), Equals(0x02));
            AssertThat(send[0].size(), Equals<uint>(0));

        });

        it("can be received", [](){
            auto recv_msg = msg::read(msg::send(msg::ping::make()));
            boost::get<msg::ping>(recv_msg);
        });

        it("can be turned into reconnect messages", [](){
            auto ping = msg::ping::make();
            auto recn = msg::reconnect::make(std::move(ping));
        });
    });

    describe("register messages", [](){
        it("can be created", [](){
            auto reg = msg::registration::make("test");
            AssertThat(reg.service(), Equals("test"));
        });
        it("can be sent", [](){
            auto reg = msg::registration::make("file");
            auto send = msg::send(std::move(reg));

            AssertThat(send, HasLength(4));
            AssertThat(*send[2].data<uint8_t>(), Equals(0x01));
            AssertThat(msg2str(send[3]), Equals("file"));
        });
        it("can be received", [](){
            auto recv_msg = msg::read(msg::send(msg::registration::make("file")));
            auto & message = boost::get<msg::registration>(recv_msg);
            AssertThat(message.service(), Equals("file"));
        });
    });

    describe("pong messages", [](){
        it("can be sent", [](){
            auto pong = msg::send(msg::pong::make(msg::ping::make()));
            AssertThat(pong, HasLength(3));
            AssertThat(*pong[2].data<uint8_t>(), Equals(0x03));
            AssertThat(pong[0].size(), Equals<uint>(0));
        });
        it("can be received", [](){
            auto pong =  msg::read(msg::send(msg::pong::make(msg::ping::make())));
            boost::get<msg::pong>(pong);
        });
    });

    describe("reply messages",[](){
        it("can be sent",[](){
            auto rep = msg::reply::make(msg::request::make("service",
                                                           msg_vec({"meta"}),
                                                           msg_vec({"data", "more data"})));
            auto send = msg::send(std::move(rep));
            AssertThat(send, HasLength(8));
            AssertThat((uint)*send[2].data<uint8_t>(), Equals<uint>(0x05));
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
        it("can be sent",[](){
            auto req = msg::request::make("service",
                                          msg_vec({"meta"}),
                                          msg_vec({"data", "more data"}));
            auto send = msg::send(std::move(req));
            AssertThat(send, HasLength(9));
            AssertThat(*send[2].data<uint8_t>(), Equals(0x04));
            AssertThat(msg2str(send[3]), Equals("service"));
        });
        it("can be received",[](){
            auto req =  msg::read(msg::send(msg::request::make("service",
                                                               msg_vec({"meta"}),
                                                               msg_vec({"data", "more data"}))));
            auto & message = boost::get<msg::request>(req);
            AssertThat(message.service(), Equals("service"));
            AssertThat(msg2str(message.metadata()[0]), Equals("meta"));
            AssertThat(msg2str(message.data()[0]), Equals("data"));
            AssertThat(msg2str(message.data()[1]), Equals("more data"));
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
};
