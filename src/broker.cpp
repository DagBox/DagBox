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
#include "broker.hpp"
#include <spdlog/spdlog.h>


auto logger = spdlog::stdout_color_st("broker");


template <class message>
auto get_addr_ensure(message & msg) -> msg::address
{
    auto maybe_addr = msg.address();
    if (!maybe_addr) {
        // The sender address part is added automatically by the
        // socket. It should always exist here.
        throw exception::fatal("Broker recieved a message with no sender");
    }
    return *maybe_addr;
}


// Pop an element from the set and return it. Which element will be
// popped is undefined. If the set is empty, calling this function on
// it is undefined behaviour.
template <class T>
auto pop_any(std::unordered_set<T> & set) -> T &&
{
    auto iter = begin(set);
    auto elem = std::move(*iter);
    set.erase(iter);
    return std::move(elem);
}


broker::broker(zmq::context_t & ctx, std::string const & addr)
    : sock(ctx, socket_type)
{
    sock.bind(addr);
}


auto broker::run(zmq::context_t & ctx, const std::string & addr) -> void
{
    broker br(ctx, addr);
    // Enter an infinite loop, processing messages
    while (true) {
        auto message = msg::read(br.sock.recv_multimsg());
        boost::apply_visitor(br, message);

        // Processing the message may result in 0 or more messages
        // that need to be sent
        while (br.send_queue.size() > 0) {
            br.sock.send_multimsg(std::move(br.send_queue.front()));
            br.send_queue.pop();
        }
    }
}


auto broker::free_worker(detail::worker const & worker) -> void
{
    auto & pending = pending_requests[worker.service];
    if (pending.size() == 0) {
        // No pending work, add to free_workers to wait for work to arrive
        free_workers[worker.service].insert(worker.address);
        return;
    } else {
        // Pending work, immediately assign the work
        auto request = std::move(pending.front());
        pending.pop();
        send_queue.push(msg::send(request));
    }
}


auto broker::operator()(msg::registration & msg) -> void
{
    auto serv = msg.service();
    auto addr = get_addr_ensure(msg);
    workers[addr] = {
        .address = addr,
        .service = serv,
        .last_seen = detail::time_now(),
    };
    send_queue.push(msg::send(msg));
    free_worker(workers[addr]);
}


auto broker::operator()(msg::ping & msg) -> void
{
    auto addr = get_addr_ensure(msg);
    auto worker_ = workers.find(addr);
    if (worker_ == workers.end()) {
        // The worker isn't registered, ask it to re-register
        send_queue.push(msg::send(msg::reconnect::make(std::move(msg))));
    } else {
        auto & worker = worker_->second;
        worker.last_seen = detail::time_now();
        send_queue.push(msg::send(msg::pong::make(std::move(msg))));
    }
}


auto broker::operator()(msg::pong & msg) -> void
{
    auto addr = get_addr_ensure(msg);
    workers[addr].last_seen = detail::time_now();
}


auto broker::operator()(msg::request & msg) -> void
{
    // If the client part is missing, then the message was sent by
    // a client, add their address to be able to return the reply
    auto addr = get_addr_ensure(msg);
    if (!msg.client()) {
        msg.client(addr);
    }
    // If the request came from a worker, mark the worker as free
    auto maybe_worker = workers.find(addr);
    if (maybe_worker != workers.end()) {
        free_worker(maybe_worker->second);
    }
    // Are there any workers who provide this service?
    auto serv = msg.service();
    auto maybe_workers = free_workers.find(serv);
    if (maybe_workers == free_workers.end()) {
        logger->warn("Recieved request for service {} "
                     "which is provided by no workers",
                     serv);
        return;
    }
    auto & available_workers = maybe_workers->second;

    // If there are no workers available, queue the work
    if (available_workers.size() == 0) {
        pending_requests[serv].push(std::move(msg));
        return;
    } else { // There is a worker available, send the work immediately
        auto worker = pop_any(available_workers);
        msg.address(worker);
        send_queue.push(msg::send(msg));
    }
}


auto broker::operator()(msg::reply & msg) -> void
{
    // Mark the worker who sent the reply as free
    auto addr = get_addr_ensure(msg);
    auto & worker = workers[addr];
    worker.last_seen = detail::time_now();
    free_worker(worker);
    // Send the reply to the client
    auto client = msg.client();
    if (!client) {
        // A reply must have a client field when recieved by the
        // broker.
        throw msg::exception::malformed("Recieved a reply that has no client");
    }
    msg.address(*client);
    send_queue.push(msg::send(msg));
}


auto broker::operator()(msg::reconnect & msg) -> void
{
    logger->warn("Recieved a reconnect message, which is for workers only");
}
