cmake_minimum_required(VERSION 3.14)

project(fabric-rpc)

# import protobuf
message(STATUS "fetching protobuf")
include(FetchContent)
FetchContent_Declare(
        protobuf
        GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
        GIT_TAG        v3.21.3
)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_CONFORMANCE OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(protobuf_WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "" FORCE)
set(protobuf_BUILD_LIBPROTOC ON CACHE BOOL "" FORCE)
if(NOT protobuf_POPULATED)
  FetchContent_Populate(protobuf)
  add_subdirectory(${protobuf_SOURCE_DIR} ${protobuf_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

message(STATUS "fetching service-fabric-cpp")

include(FetchContent)
FetchContent_Declare(service_fabric_cpp
    GIT_REPOSITORY https://github.com/youyuanwu/service-fabric-cpp.git
    GIT_TAG d0bed363785fdab87655c3bdbe6deba13b94f874
)
FetchContent_GetProperties(service_fabric_cpp)
if(NOT service_fabric_cpp_POPULATED)
    FetchContent_Populate(service_fabric_cpp)
    add_subdirectory(${service_fabric_cpp_SOURCE_DIR} ${service_fabric_cpp_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

include(${service_fabric_cpp_SOURCE_DIR}/cmake/clang-format.cmake)

add_subdirectory(src)
add_subdirectory(examples)

enable_testing()
add_subdirectory(tests)