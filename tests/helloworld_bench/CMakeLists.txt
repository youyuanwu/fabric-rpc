# benchmark test for hello world implementation

# using the same hello world lib from example folder.
find_package(Boost REQUIRED COMPONENTS program_options) 

add_executable(helloworld_bench bench_main.cpp)

target_link_libraries(helloworld_bench
  PRIVATE
  lib_helloworld
  fabric_rpc_proto
  fabric_rpc
  fabric_rpc_tool
  Boost::unit_test_framework Boost::disable_autolinking
  Boost::program_options
)

target_include_directories(
  helloworld_bench
  PRIVATE
  .
  ../base_test
)

target_compile_definitions(helloworld_bench
  PUBLIC WIN32_LEAN_AND_MEAN # This is to get rid of include from fabric of winsock.h for asio
)

add_test(NAME helloworld_bench COMMAND helloworld_bench
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})