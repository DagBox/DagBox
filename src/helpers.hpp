#pragma once

#include <chrono>


namespace detail
{
    typedef std::chrono::time_point<std::chrono::steady_clock> time;

    auto inline time_now() -> time
    {
        return std::chrono::steady_clock::now();
    }
}
