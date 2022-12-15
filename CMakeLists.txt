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
    GIT_TAG ca34352223600d701b2ec40db4d31167469e1035
)
FetchContent_GetProperties(service_fabric_cpp)
if(NOT service_fabric_cpp_POPULATED)
    FetchContent_Populate(service_fabric_cpp)
    add_subdirectory(${service_fabric_cpp_SOURCE_DIR} ${service_fabric_cpp_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

include(${service_fabric_cpp_SOURCE_DIR}/cmake/clang-format.cmake)

add_subdirectory(src)

# nuget package
add_custom_target(nuget_pack
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND nuget pack FabricRpc.nuspec 
    -OutputDirectory ${CMAKE_CURRENT_BINARY_DIR}/packages
)

add_custom_target(nuget_inspect
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND ${CMAKE_COMMAND} -E rm -rf build/nuget_inspect
  COMMAND nuget install FabricRpc 
    -Source ${CMAKE_CURRENT_BINARY_DIR}/packages
    -OutputDirectory build/nuget_inspect
)

add_subdirectory(examples)

enable_testing()
add_subdirectory(tests)