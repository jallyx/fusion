include_directories(../feature)
aux_source_directory(. SRC_LIST)
add_library(external STATIC ${SRC_LIST})
set(CMAKE_CXX_FLAGS "-Wall -std=c++11 -pthread")
