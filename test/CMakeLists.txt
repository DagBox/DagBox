add_executable(tests tests.cpp)
target_link_libraries(tests zmq socket message)

add_test(tests tests)
SETUP_TARGET_FOR_COVERAGE(test-coverage tests coverage)
