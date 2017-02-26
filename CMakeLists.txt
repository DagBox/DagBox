cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(DagBox CXX)

# Add the test framework
include_directories(./submodules/bandit)
enable_testing()

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

add_subdirectory(src)
add_subdirectory(test)
