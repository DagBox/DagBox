add_library(socket STATIC socket.cpp)

add_executable(hello hello.cpp)
target_link_libraries(hello zmq lmdb boost_context  socket)
