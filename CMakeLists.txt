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

message(STATUS "fetching fabric_metadata")

include(FetchContent)
FetchContent_Declare(fabric_metadata
    GIT_REPOSITORY https://github.com/youyuanwu/fabric-metadata.git
    GIT_TAG d9818619b922b947124b33f7fe80ebca996be4db
)
FetchContent_GetProperties(fabric_metadata)
if(NOT fabric_metadata_POPULATED)
    FetchContent_Populate(fabric_metadata)
    add_subdirectory(${fabric_metadata_SOURCE_DIR} ${fabric_metadata_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

include(${fabric_metadata_SOURCE_DIR}/cmake/clang-format.cmake)

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

# coverage
find_program(OpenCppCoverage_exe
    NAMES OpenCppCoverage.exe
)
if(OpenCppCoverage_exe)
    message(STATUS "coverage tool found: ${OpenCppCoverage_exe}")
    # coverage tool only recognizes windows style path, backslash.
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" PWD_WIN_PATH)
    add_custom_target(coverage
        COMMAND ${OpenCppCoverage_exe} --quiet --export_type cobertura:cobertura.xml --cover_children 
        --sources "${PWD_WIN_PATH}\\src\\fabric_rpc" --modules "${PWD_WIN_PATH}"
        -- ctest -C Debug --test-dir build --repeat until-pass:3 --timeout 30
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
else()
    message(STATUS "coverage tool not found: ${OpenCppCoverage_exe}")
endif()