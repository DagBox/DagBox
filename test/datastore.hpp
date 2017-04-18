/*
  Copyright 2017 Kaan GenÃ§, Melis Narin Kaya

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
            auto testdir = filesystem::temp_directory_path() / filesystem::unique_path();
            filesystem::create_directory(testdir);

            data::storage store(testdir);
            data::writer writer(store);
            data::reader reader(store);

            it("can write data", [&](){
                auto reply_msg = msg::read(writer(msg::request::make(
                                                      "datastore writer", {},
                                                      // A write request, inserting "test_user_data" into bucket "users"
                                                      msg_vec({"‚¦bucket¥users¤data®test_user_data"}))));
                auto & reply = boost::get<msg::reply>(reply_msg);
                // Inserted 1 object, we should get back 1 key
                AssertThat(reply.data(), HasLength(1));
                // Make sure the key is not empty
                AssertThat(reply.data()[0].size(), IsGreaterThan<unsigned int>(0));
            });
        });
    });
};
