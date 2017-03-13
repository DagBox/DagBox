cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(DagBox CXX)

# Enable code coverage for debug builds
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/submodules/cmake-modules")
INCLUDE(CodeCoverage)
set(LCOV_REMOVE_EXTRA '*/test/*' '*/bandit/*')
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -o0 -fprofile-arcs -ftest-coverage")

# Add the test framework
include_directories(./submodules/bandit)
enable_testing()

# Generate documentation
include(DoxygenTargets)
add_doxygen(./Doxyfile
  OUTPUT_DIRECTORY "docs"
  DOC_TARGET "docs"
  NO_PDF)

# Add the logging library
include_directories(./submodules/spdlog/include)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

add_subdirectory(src)
add_subdirectory(test)
