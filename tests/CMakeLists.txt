set(Boost_USE_STATIC_LIBS ON) # use static boost
find_package(Boost REQUIRED COMPONENTS unit_test_framework )


set (CMAKE_CXX_STANDARD 20)

set(_test_name com_test)

add_executable(${_test_name} ${_test_name}.cpp)

target_link_libraries(${_test_name}
    PUBLIC Boost::unit_test_framework Boost::disable_autolinking
    PUBLIC fabric_headers
    PUBLIC fabric_rpc
)

add_test(NAME ${_test_name} COMMAND ${_test_name}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(fabric_rpc_base_test fabric_rpc_base_test.cpp)
target_link_libraries(fabric_rpc_base_test
  PUBLIC Boost::unit_test_framework Boost::disable_autolinking
  PUBLIC fabric_headers
  PUBLIC fabric_rpc
  PUBLIC fabric_rpc_proto
)
add_test(NAME fabric_rpc_base_test COMMAND fabric_rpc_base_test
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(todolist)
add_subdirectory(helloworld_bench)