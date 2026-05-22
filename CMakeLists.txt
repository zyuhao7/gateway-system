cmake_minimum_required(VERSION 3.20)
project(GatewaySystem CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fsanitize=address")

find_package(Boost 1.83 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(gateway_server
    src/main.cpp
    src/server.cpp
    src/connection.cpp
    src/connection_manager.cpp
    src/io_context_pool.cpp
)

target_link_libraries(gateway_server
    ${Boost_LIBRARIES}
    Threads::Threads
)
