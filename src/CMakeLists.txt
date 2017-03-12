add_library(socket STATIC socket.cpp)

add_library(message STATIC message.cpp)
target_link_libraries(message)

add_library(broker STATIC broker.cpp)
target_link_libraries(broker message)

add_library(worker STATIC worker.cpp)
target_link_libraries(worker message)
