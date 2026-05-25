cmake_minimum_required(VERSION 3.20)
project(GatewaySystem CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fsanitize=address")

find_package(Boost 1.83 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

# Find hiredis
find_library(HIREDIS_LIB hiredis)
if(NOT HIREDIS_LIB)
    message(FATAL_ERROR "hiredis library not found")
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(gateway_server
    src/main.cpp
    src/server.cpp
    src/connection.cpp
    src/connection_manager.cpp
    src/io_context_pool.cpp
    src/consistent_hash.cpp
    src/node_registry.cpp
    src/node_registry_redis.cpp
    src/message_router.cpp
    src/metrics.cpp
    src/metrics_server.cpp
)

target_link_libraries(gateway_server
    ${Boost_LIBRARIES}
    Threads::Threads
    ${HIREDIS_LIB}
)

# Test executable for Redis integration
add_executable(test_redis_integration
    test_redis_integration.cpp
    src/node_registry.cpp
    src/node_registry_redis.cpp
    src/consistent_hash.cpp
)

target_link_libraries(test_redis_integration
    Threads::Threads
    ${HIREDIS_LIB}
)
