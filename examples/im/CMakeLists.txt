# IM Server Example
cmake_minimum_required(VERSION 3.15)

# 包含父项目的头文件
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(im_server
    src/im_server.cpp
    ${CMAKE_SOURCE_DIR}/src/auth.cpp
    ${CMAKE_SOURCE_DIR}/src/rate_limiter.cpp
    ${CMAKE_SOURCE_DIR}/src/room.cpp
    ${CMAKE_SOURCE_DIR}/src/protocol.cpp
    ${CMAKE_SOURCE_DIR}/src/message_storage.cpp
)

target_link_libraries(im_server
    ${Boost_LIBRARIES}
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    ${ROCKSDB_LIB}
)

# 安装
install(TARGETS im_server DESTINATION bin)
