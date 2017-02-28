add_library(socket STATIC socket.cpp)

add_library(message STATIC message.cpp)
target_link_libraries(message ${Boost_CONTEXT_LIBRARY})

add_executable(hello hello.cpp)
target_link_libraries(hello zmq lmdb ${Boost_CONTEXT_LIBRARY}  socket message)
