add_library(socket STATIC socket.cpp)
add_library(message STATIC message.cpp)

find_package(Boost COMPONENTS context)

add_executable(hello hello.cpp)
target_link_libraries(hello zmq lmdb ${Boost_CONTEXT_LIBRARY}  socket message)
