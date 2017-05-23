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

#include <chrono>
#include <thread>
#include <atomic>


/*! \file helpers.hpp
 * Small classes and utilities that don't fit in other files.
 */



namespace detail_time
{
    typedef std::chrono::steady_clock clock;
    typedef std::chrono::time_point<clock> time;

    auto inline time_now() -> time
    {
        return std::chrono::steady_clock::now();
    }
}


/*! \brief Run a component on a thread.
 *
 * Creates a thread and runs a component in that thread. The component
 * is a class with a public constructor and a `run(void)` method. The
 * `run` method will be continuously called until the component is
 * destructed.
 */
template <class C>
class component
{
    std::thread thread;
    std::atomic_bool condition;

    // Components can't be trivially copied
    component(component const &) = delete;
    auto operator=(component const &) -> component & = delete;
public:
    /*! \brief Create a component.
     *
     * \param args These arguments will be directly passed to the
     * constructor of class `C`.
     */
    template <class ...Args> component(Args && ... args)
        : condition(true)
    {
        thread = std::thread([&](){
            C comp(args...);
            while (condition.load()) {
                comp.run();
            }
        });
    }

    component(component && c) noexcept
    {
        condition.store(c.condition);
        thread = std::move(c.thread);
    }

    auto operator=(component && c) noexcept -> component &
    {
        condition.store(c.condition);
        thread = std::move(c.thread);
    }

    ~component()
    {
        condition.store(false);
        thread.join();
    }
};
