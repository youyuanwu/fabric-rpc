set(Boost_USE_STATIC_LIBS ON) # use static boost
find_package(Boost REQUIRED COMPONENTS unit_test_framework )


set (CMAKE_CXX_STANDARD 20)

add_executable(fabric_rpc_base_test 
  fabric_rpc_base_test.cpp
  com_test.cpp
  async_test.cpp
  timeout_test.cpp
  winrt_test.cpp
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

add_subdirectory(todolist)
add_subdirectory(helloworld_bench)