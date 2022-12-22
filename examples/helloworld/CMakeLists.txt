set (CMAKE_CXX_STANDARD 20)

set(Boost_NO_WARN_NEW_VERSIONS 1)
set(Boost_USE_STATIC_LIBS ON) # use static boost
find_package(Boost REQUIRED COMPONENTS log) # log is used for tests

add_compile_definitions(
  _WIN32_WINNT=0x0601
)

set(_proto_file_path helloworld.proto)

include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${_proto_file_path})

protobuf_generate(LANGUAGE grpc
    PLUGIN "protoc-gen-grpc=$<TARGET_FILE:fabric_rpc_cpp_plugin>"
    OUT_VAR FABRIC_RPC_SRCS
    APPEND_PATH
    GENERATE_EXTENSIONS
        .fabricrpc.h
        .fabricrpc.cc
    PROTOS ${_proto_file_path}
)


# common lib
set(_hello_lib lib_helloworld)
add_library(${_hello_lib} 
  helloworld-tools.ixx
#  helloworld.gen.cpp
#  helloworld.gen.h
${PROTO_SRCS} ${PROTO_HDRS}
${FABRIC_RPC_SRCS})

# this is not needed. But the developing incremental build is not working yet.
#add_dependencies( fabric_rpc_cpp_plugin)

# for generated helloworld proto cpp
target_include_directories(${_hello_lib} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${_hello_lib} 
  PUBLIC fabric_rpc_proto
    fabric_rpc
    fabric_internal_headers
    fabric_internal_uuids
    FabricTransport
    fabric_error
    fabric_ext
    fabric_internal_ext
    Boost::log Boost::disable_autolinking Boost::headers
)

# server
set(_server_exe hello_server)
add_executable(${_server_exe} main.cpp)
target_link_libraries(${_server_exe} PRIVATE ${_hello_lib})

# client
set(_client_exe hello_client)
add_executable(${_client_exe} client.cpp)
target_link_libraries(${_client_exe} PRIVATE ${_hello_lib})

# target to replace generated code.
add_custom_target(hello_world_replace_manual_code
  DEPENDS ${FABRIC_RPC_SRCS}
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/helloworld.fabricrpc.h ${CMAKE_CURRENT_SOURCE_DIR}/helloworld.gen.h
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/helloworld.fabricrpc.cc ${CMAKE_CURRENT_SOURCE_DIR}/helloworld.gen.cpp
)