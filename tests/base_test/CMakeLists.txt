file(GLOB_RECURSE _SOURCES
  *.cpp
  *.hpp
)

add_executable(fabric_rpc_base_test 
  ${_SOURCES}
)
target_link_libraries(fabric_rpc_base_test
  PUBLIC Boost::unit_test_framework Boost::disable_autolinking
  fabric_internal_sdk
  fabric_rpc
  fabric_rpc_proto
  lib_helloworld
  fabric_rpc_tool
)
add_test(NAME fabric_rpc_base_test COMMAND fabric_rpc_base_test
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})