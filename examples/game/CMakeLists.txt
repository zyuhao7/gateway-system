# Game Server Example
cmake_minimum_required(VERSION 3.15)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(game_server
    src/game_server.cpp
    ${CMAKE_SOURCE_DIR}/src/room.cpp
    ${CMAKE_SOURCE_DIR}/src/rate_limiter.cpp
)

target_link_libraries(game_server
    ${Boost_LIBRARIES}
    Threads::Threads
)

install(TARGETS game_server DESTINATION bin)
