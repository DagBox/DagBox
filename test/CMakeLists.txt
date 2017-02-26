find_package(Boost COMPONENTS context)

add_executable(test-socket message.cpp)
target_link_libraries(test-socket zmq socket)

add_executable(test-message message.cpp)
target_link_libraries(test-message zmq)

add_test(test-socket test-socket)
add_test(test-message test-message)
