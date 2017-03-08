#pragma once

#include <string>
#include <vector>
#include <initializer_list>

#include <bandit/bandit.h>
using namespace bandit;
using namespace snowhouse;

#include <zmq.hpp>



auto msg2str(zmq::message_t const & msg) -> std::string
{
    return std::string(msg.data<char>(), msg.size());
}


auto msg_vec(std::initializer_list<std::string> msgs)
    -> std::vector<zmq::message_t>
{
    std::vector<zmq::message_t> vec;
    for (auto msg : msgs) {
        if (msg.size() == 0) {
            vec.push_back(zmq::message_t());
        } else {
            vec.push_back(zmq::message_t(msg.data(), msg.size()));
        }
    }
    return vec;
}
