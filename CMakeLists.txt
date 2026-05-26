cmake_minimum_required(VERSION 3.20)
project(GatewaySystem CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fsanitize=address")

find_package(Boost 1.83 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(GTest REQUIRED)

# Find hiredis
find_library(HIREDIS_LIB hiredis)
if(NOT HIREDIS_LIB)
    message(FATAL_ERROR "hiredis library not found")
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})  # For generated proto files

# Generate gRPC code from proto files
set(PROTO_FILES proto/gateway.proto)
set(PROTO_SRCS ${CMAKE_CURRENT_BINARY_DIR}/proto/gateway.pb.cc)
set(PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR}/proto/gateway.pb.h)
set(GRPC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/proto/gateway.grpc.pb.cc)
set(GRPC_HDRS ${CMAKE_CURRENT_BINARY_DIR}/proto/gateway.grpc.pb.h)

add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/proto
    COMMAND protobuf::protoc
    ARGS --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
         --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
         --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
         -I${CMAKE_SOURCE_DIR}
         ${PROTO_FILES}
    DEPENDS ${PROTO_FILES}
    COMMENT "Generating gRPC code"
)

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
    src/grpc_gateway_service.cpp
    src/grpc_gateway_client.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_link_libraries(gateway_server
    ${Boost_LIBRARIES}
    Threads::Threads
    ${HIREDIS_LIB}
    protobuf::libprotobuf
    gRPC::grpc++
    gRPC::grpc++_reflection
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Test executable for Redis integration
add_executable(test_redis_integration
    tests/test_redis_integration.cpp
    src/node_registry.cpp
    src/node_registry_redis.cpp
    src/consistent_hash.cpp
)

target_link_libraries(test_redis_integration
    Threads::Threads
    ${HIREDIS_LIB}
)

# Test executable for gRPC communication
add_executable(test_grpc_communication
    tests/test_grpc_communication.cpp
    src/grpc_gateway_service.cpp
    src/grpc_gateway_client.cpp
    src/connection_manager.cpp
    src/connection.cpp
    src/metrics.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_link_libraries(test_grpc_communication
    ${Boost_LIBRARIES}
    Threads::Threads
    protobuf::libprotobuf
    gRPC::grpc++
    gRPC::grpc++_reflection
)

# Test executable for SSL/TLS server
add_executable(test_ssl_server
    tests/test_ssl_server.cpp
    src/ssl_server.cpp
    src/io_context_pool.cpp
)

target_link_libraries(test_ssl_server
    ${Boost_LIBRARIES}
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Unit test for Metrics
add_executable(test_metrics
    tests/test_metrics.cpp
    src/metrics.cpp
)

target_link_libraries(test_metrics
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

# Unit test for ConsistentHash
add_executable(test_consistent_hash
    tests/test_consistent_hash.cpp
    src/consistent_hash.cpp
)

target_link_libraries(test_consistent_hash
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

# Enable testing
enable_testing()
add_test(NAME MetricsTest COMMAND test_metrics)
add_test(NAME ConsistentHashTest COMMAND test_consistent_hash)
