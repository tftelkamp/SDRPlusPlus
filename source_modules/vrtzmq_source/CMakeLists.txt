cmake_minimum_required(VERSION 3.13)
project(vrtzmq_source)

find_package(ZMQ REQUIRED)
find_package(VRT REQUIRED)
include_directories(${ZMQ_INCLUDE_DIRS} ${VRT_INCLUDE_DIRS})
file(GLOB SRC "src/*.cpp")

include(${SDRPP_MODULE_CMAKE})

target_include_directories(vrtzmq_source PRIVATE "src/" ${ZMQ_INCLUDE_DIRS} ${VRT_INCLUDE_DIRS})

target_link_libraries(vrtzmq_source PRIVATE ${ZMQ_LIBRARIES} ${VRT_LIBRARIES})
