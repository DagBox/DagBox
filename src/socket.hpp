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

#include <vector>
#include <tuple>
#include <zmq.hpp>

using std::begin;
using std::end;



/*! \file socket.hpp
 * Extensions on ZeroMQ's socket.
 *
 * We extend socket to be able to easily send and recieve multi-part
 * messages.
 */



/*! A ZeroMQ socket that can recieve messages containing multiple
 *  parts.
 */
class socket : public zmq::socket_t
{
public:
    // Just use regular socket's constructors
    using socket_t::socket_t;

    /*! \brief Recieve a message that has multiple parts as a stream.
     */
    auto recv_multimsg() -> std::vector<zmq::message_t>;

    /*! \brief Send a message that has multiple parts.
     */
    template <class container>
    auto send_multimsg(container && cont) -> void
    {
        typedef typename container::iterator iterator;
        iterator iter = begin(cont);
        iterator last = end(cont);
        while (iter != last) {
            auto part = std::move(*iter);
            ++iter;
            if (iter != last) { // More parts to send
                send(part, ZMQ_SNDMORE);
            } else {
                send(part);
            }
        }
    }

};
