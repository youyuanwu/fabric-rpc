
set(_proto_file_path todolist.proto)

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
add_executable(todolist_test
    main.cpp
    server.hpp
    client.hpp
${PROTO_SRCS} ${PROTO_HDRS}
${FABRIC_RPC_SRCS}
)

target_link_libraries(todolist_test PUBLIC
    fabric_rpc_proto
    fabric_rpc
    fabric_internal_headers
    fabric_internal_uuids
    FabricTransport
    fabric_error
    fabric_ext
    fabric_internal_ext
    #libprotobuf
    Boost::unit_test_framework Boost::disable_autolinking
)

target_include_directories(todolist_test PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    .. # for test helper
)

add_test(NAME todolist_test COMMAND todolist_test
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

