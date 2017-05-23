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


#include "helpers.hpp"
#include "assistant.hpp"
#include "broker.hpp"
#include "worker/datastore.hpp"
#include "worker/lock.hpp"
#include "socket.hpp"


/*! \file client.hpp
 * Client library to help embed DagBox.
 */



/*! \brief Client library to help embed DagBox.
 */
namespace dgbx
{
    enum class transport
    {
        inprocess,
        ipc,
        tcp,
    };

    /*! \brief DagBox database system.
     *
     * This is a class representing the entire database. It contains
     * the broker and all workers.
     */
    class dagbox
    {
    public:
        /*! \brief The address of the broker.
         *
         * The client library, and workers started outside this class
         * will need to connect to this address to access the system.
         */
        std::string const address;
    private:
        std::chrono::milliseconds const worker_timeout;
        zmq::context_t context;
        data::storage storage;

        component<broker> broker;
        component<assistant<data::writer>> writer;
        std::vector<component<assistant<data::reader>>> readers;

        dagbox() = delete;
        dagbox(dagbox const &) = delete;
        auto operator=(dagbox const &) -> dagbox & = delete;
    public:
        /*! \brief Start DagBox.
         *
         * \param data_directory The directory that will be used by
         * DagBox to store data. The application must have write
         * access here.
         *
         * \param transport Transport method that will be used by
         * DagBox. If your application will use multiple processes,
         * use [\ref transport::ipc]. If it will run on multiple
         * machines across the network, use [\ref transport::tcp].
         * Otherwise, [\ref transport::inprocess] should provide the
         * best performance.
         *
         * \param broker_address The address that the broker will
         * listen on. If this is an empty string, a unique random
         * address will be generated. You can pass a specific address
         * here if you'd prefer to have a fixed address. Do not
         * include the transport part of the address, only the name
         * part.
         *
         * \param reader_count The number of data reader workers that
         * will be started. Increasing this value can help utilize
         * disk throughput more effectively, at the cost of using more
         * threads.
         *
         * \param worker_timeout The amount of time after which a
         * worker should be considered dead if it hasn't communicated
         * with the broker. This value should be larger than the time
         * an average request would take.
         *
         * \param transport_delay The amount of time a message
         * generally takes to be transported through the chosen
         * transport. This value is used by the broker to give more
         * time to workers for their responses to reach before marking
         * them as dead.
         */
        dagbox(
            filesystem::path data_directory,
            enum transport transport=transport::inprocess,
            std::string broker_address="",
            uint reader_count=4,
            std::chrono::milliseconds worker_timeout=std::chrono::milliseconds{500},
            std::chrono::milliseconds transport_delay=std::chrono::milliseconds{100});

        /*! \brief Create more data reader workers.
         *
         * \param count The number of workers to add.
         */
        auto reader_add(uint count=1) -> void;
        /*! \brief Kill some data reader workers.
         *
         * The most recently created ones will be killed first.
         *
         * \param count The number of workers to kill.
         */
        auto reader_remove(uint count=1) -> void;
        /*! \brief Count the number of data reader workers that are active. */
        auto reader_count() const -> uint;
    };
};
