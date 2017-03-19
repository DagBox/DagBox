add_executable(tests tests.cpp)
target_link_libraries(tests zmq pthread socket message broker)

add_test(tests tests)
SETUP_TARGET_FOR_COVERAGE(test-coverage tests coverage)
