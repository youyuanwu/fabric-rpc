
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ../protos/fabricrpc.proto)

set(_lib_name fabric_rpc_proto)

add_library(${_lib_name} STATIC
  ${PROTO_SRCS} ${PROTO_HDRS}
)

target_include_directories(${_lib_name}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${_lib_name} PUBLIC libprotobuf)

# fabric rpc impl lib
add_library(fabric_rpc STATIC
  Status.cpp
  FRPCRequestHandler.cpp
  FRPCTransportMessage.cpp
  FRPCHeader.cpp
)

# turn on all warnings
target_compile_options(fabric_rpc PUBLIC /W4 /WX)

target_include_directories(fabric_rpc PUBLIC ../include)

target_link_libraries(fabric_rpc PUBLIC 
  FabricTransport 
  fabric_internal_headers)

add_subdirectory(generator)

# fabric-rpc-tool lib used for testing and not intended to be used/shipped outside this repo.
add_subdirectory(tool)