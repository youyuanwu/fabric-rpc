add_compile_definitions(
  _WIN32_WINNT=0x0601
)

set(_proto_file_path ../helloworld/helloworld.proto)

include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${_proto_file_path})

protobuf_generate(LANGUAGE grpc
    PLUGIN "protoc-gen-grpc=$<TARGET_FILE:fabric_rpc2_cpp_plugin>"
    OUT_VAR FABRIC_RPC_SRCS
    APPEND_PATH
    GENERATE_EXTENSIONS
        .fabricrpc2.h
        .fabricrpc2.cc
    PROTOS ${_proto_file_path}
)


# common lib
set(_hello_lib lib_helloworld2)
add_library(${_hello_lib} 
${PROTO_SRCS} ${PROTO_HDRS}
${FABRIC_RPC_SRCS})

# for generated helloworld proto cpp
target_include_directories(${_hello_lib} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${_hello_lib} 
  PUBLIC fabric_rpc_proto
    fabric_rpc2
    fabric_internal_sdk
    FabricTransport
)


# server
set(_server_exe hello_server2)
add_executable(${_server_exe} server2.cpp)
target_link_libraries(${_server_exe} PRIVATE
  PRIVATE 
  lib_helloworld2
  fabric_rpc_proto
  fabric_rpc2
  fabric_rpc_tool
  absl::status
)

# client
set(_client_exe hello_client2)
add_executable(${_client_exe} client2.cpp)
target_link_libraries(${_client_exe} 
  PRIVATE 
  lib_helloworld2
  fabric_rpc_proto
  fabric_rpc2
  fabric_rpc_tool
  absl::status
)
