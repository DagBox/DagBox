add_library(datastore STATIC datastore.cpp)
target_link_libraries(datastore zmq lmdb ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})

add_library(lock STATIC lock.cpp)
