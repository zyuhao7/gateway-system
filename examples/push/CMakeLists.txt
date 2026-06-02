# Push Server Example
cmake_minimum_required(VERSION 3.15)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(push_server
    src/push_server.cpp
    ${CMAKE_SOURCE_DIR}/src/rate_limiter.cpp
)

target_link_libraries(push_server
    ${Boost_LIBRARIES}
    Threads::Threads
)

install(TARGETS push_server DESTINATION bin)
