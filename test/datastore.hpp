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
#include "../src/worker/datastore.hpp"


auto test_datastore = [](){
    describe("datastore", [](){
        describe("writer", [](){
            auto testdir = filesystem::temp_directory_path() / filesystem::unique_path("DagBox-test-%%%%-%%%%-%%%%-%%%%");
            filesystem::create_directory(testdir);

            data::storage store(testdir);
            data::writer writer(store);
            data::reader reader(store);

            it("can write data", [&](){
                data::detail::write_request wreq = {
                    .bucket = "users",
                    .data = "test_data",
                };
                auto write_msg = msg::read(writer(msg::request::make(
                                                      "datastore writer", {},
                                                      // A write request, inserting "test_data" into bucket "users"
                                                      msg_vec({dumps(wreq)}))));
                auto & write_reply = boost::get<msg::reply>(write_msg);
                // Inserted 1 object, we should get back 1 key
                AssertThat(write_reply.data(), HasLength(1));
                // Make sure the key is not empty
                AssertThat(write_reply.data()[0].size(), IsGreaterThan<unsigned int>(0));
                auto key = loads<std::string>(write_reply.data()[0]);

                data::detail::read_request rreq = {
                    .bucket = "users",
                    .key = key,
                    .data = boost::none,
                    .relations = {},
                };
                auto read_msg = msg::read(reader(msg::request::make(
                                                     "datastore reader", {},
                                                     // A read request that will try to read the data we just inserted
                                                     msg_vec({dumps(rreq)}))));
                auto & read_reply_msg = boost::get<msg::reply>(read_msg);
                // Requested only 1 object
                AssertThat(read_reply_msg.data(), HasLength(1));
                // Check if the data we have read is correct
                auto read_reply = loads<data::detail::read_request>(read_reply_msg.data()[0]);
                AssertThat(read_reply.bucket, Equals("users"));
                AssertThat(read_reply.key, Equals(key));
                AssertThat(*read_reply.data, Equals("test_data"));
            });
        });
    });
};
