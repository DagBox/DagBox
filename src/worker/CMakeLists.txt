add_library(datastore STATIC datastore.cpp)
target_link_libraries(datastore zmq lmdb)
