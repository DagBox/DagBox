#include "broker.hpp"


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
        // The worker isn't registered
        // TODO: Ask the worker to re-register
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
    if (!msg.client()) {
        // If the client part is missing, then the message was sent by
        // a client, add their address to be able to return the reply
        msg.client(get_addr_ensure(msg));
    }
    auto serv = msg.service();
    auto workers_ = free_workers.find(serv);
    // TODO: If the request came from a worker (the worker is bouncing
    // the work), mark the worker as free
    if (workers_ == free_workers.end()) {
        // There are no workers that provide this service
        // TODO: Return an error to the sender, or log an error, or both
        return;
    }
    auto & available_workers = workers_->second;
    if (available_workers.size() == 0) {
        // No workers available, queue the work
        pending_requests[serv].push(std::move(msg));
        return;
    } else {
        // There is a worker available, send the work immediately
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
