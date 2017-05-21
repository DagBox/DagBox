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
#include <string>
#include <bandit/bandit.h>
#include "socket.hpp"
#include "message.hpp"
#include "broker.hpp"
#include "assistant.hpp"
#include "datastore.hpp"
#include "lock.hpp"


go_bandit([](){
    test_socket();
    test_message();
    test_broker();
    test_assistant();
    test_datastore();
    test_lock();
});


auto main(int argc, char* argv[]) -> int
{
    return bandit::run(argc, argv);
}
