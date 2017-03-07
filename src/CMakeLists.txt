add_library(socket STATIC socket.cpp)

add_library(message STATIC message.cpp)
target_link_libraries(message ${Boost_CONTEXT_LIBRARY})

add_library(broker STATIC broker.cpp)
target_link_libraries(broker message ${Boost_CONTEXT_LIBRARY})
