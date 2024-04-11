
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ../protos/fabricrpc.proto)

set(_lib_name fabric_rpc_proto)

add_library(${_lib_name} STATIC
  ${PROTO_SRCS} ${PROTO_HDRS}
)

target_include_directories(${_lib_name}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${_lib_name} 
  PUBLIC protobuf::libprotobuf
)


add_subdirectory(fabric_rpc)

add_subdirectory(generator)

# experimental impl
add_subdirectory(fabric_rpc2)

add_subdirectory(generator2)

# fabric-rpc-tool lib used for testing and not intended to be used/shipped outside this repo.
add_subdirectory(tool)