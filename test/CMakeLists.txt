add_executable(test-build tests.cpp)
target_link_libraries(test-build zmq pthread socket message broker datastore lock)

add_test(test-build test-build)
SETUP_TARGET_FOR_COVERAGE(test-coverage test-build coverage)
