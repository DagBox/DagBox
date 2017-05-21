/*
  Copyright 2017 Kaan Genç

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

#include <unordered_set>
#include <msgpack.hpp>
#include "../message.hpp"


namespace lock
{
    namespace detail
    {
        struct lock_request
        {
            std::string key;
            /*! The operation to perform. If true, we will lock the
             *  key. If false, we will try to unlock.
             */
            bool lock;

            MSGPACK_DEFINE_MAP(key, lock);
        };
    };

    class lock
    {
        std::unordered_set<std::string> locks;
    public:
        /*! \brief Service name. */
        std::string const service_name = "lock";
        /*! \brief Process a request message containing a lock request. */
        auto operator()(msg::request && request) -> std::vector<zmq::message_t>;
    };
};
