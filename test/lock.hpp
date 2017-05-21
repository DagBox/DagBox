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
#include "../src/worker/lock.hpp"


auto test_lock = [](){
    describe("lock service", [](){
        lock::lock lock_serv;

        it("can lock a key", [&](){
            lock::detail::lock_request lreq = {
                .key = "test_key",
                .lock = true,
            };

            auto lock_msg = msg::read(
                lock_serv(
                    msg::request::make("lock", {},
                                       msg_vec({dumps(lreq)}))));
            auto & lock_reply = boost::get<msg::reply>(lock_msg);
            // We locked 1 key, we should get back 1 reply
            AssertThat(lock_reply.data(), HasLength(1));
            // The operation should be successful
            auto lock_status = loads<bool>(lock_reply.data()[0]);
            AssertThat(lock_status, Equals(true));
        });

        it("can't lock a key that is already locked", [&](){
            lock::detail::lock_request lreq = {
                .key = "test_key",
                .lock = true,
            };

            auto lock_msg = msg::read(
                lock_serv(
                    msg::request::make("lock", {},
                                       msg_vec({dumps(lreq)}))));
            auto & lock_reply = boost::get<msg::reply>(lock_msg);
            // We locked 1 key, we should get back 1 reply
            AssertThat(lock_reply.data(), HasLength(1));
            // The operation should fail
            auto lock_status = loads<bool>(lock_reply.data()[0]);
            AssertThat(lock_status, Equals(false));
        });

        it("can unlock a key", [&](){
            lock::detail::lock_request lreq = {
                .key = "test_key",
                .lock = false,
            };

            auto lock_msg = msg::read(
                lock_serv(
                    msg::request::make("lock", {},
                                       msg_vec({dumps(lreq)}))));
            auto & lock_reply = boost::get<msg::reply>(lock_msg);
            // We locked 1 key, we should get back 1 reply
            AssertThat(lock_reply.data(), HasLength(1));
            // The operation should succeed
            auto lock_status = loads<bool>(lock_reply.data()[0]);
            AssertThat(lock_status, Equals(true));
        });

        it("can re-lock a key that was unlocked", [&](){
            lock::detail::lock_request lreq = {
                .key = "test_key",
                .lock = true,
            };

            auto lock_msg = msg::read(
                lock_serv(
                    msg::request::make("lock", {},
                                       msg_vec({dumps(lreq)}))));
            auto & lock_reply = boost::get<msg::reply>(lock_msg);
            // We locked 1 key, we should get back 1 reply
            AssertThat(lock_reply.data(), HasLength(1));
            // The operation should succeed
            auto lock_status = loads<bool>(lock_reply.data()[0]);
            AssertThat(lock_status, Equals(true));
        });
    });
};
