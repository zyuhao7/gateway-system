# IoT Server Example
cmake_minimum_required(VERSION 3.15)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(iot_server
    src/iot_server.cpp
    ${CMAKE_SOURCE_DIR}/src/rate_limiter.cpp
    ${CMAKE_SOURCE_DIR}/src/message_storage.cpp
)

target_link_libraries(iot_server
    ${Boost_LIBRARIES}
    Threads::Threads
    ${ROCKSDB_LIB}
)

install(TARGETS iot_server DESTINATION bin)
