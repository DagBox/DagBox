add_library(socket STATIC socket.cpp)

add_library(message STATIC message.cpp)

add_library(broker STATIC broker.cpp)
target_link_libraries(broker message)


add_subdirectory(worker)
