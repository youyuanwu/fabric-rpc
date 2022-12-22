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

# target_compile_definitions(${_test_name}
#   PUBLIC WIN32_LEAN_AND_MEAN # This is to get rid of include from fabric of winsock.h
#   PUBLIC _WIN32_WINNT=0x601  # This is to pin the log version
#   PUBLIC SF_DEBUG=1
# )

# target_include_directories(${_test_name}
#   PRIVATE .
# )

# boost log leaks memory as intended, so do not check mem leaks.
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

# target_compile_definitions(fabric_rpc_base_test
#   PUBLIC WIN32_LEAN_AND_MEAN
#   PUBLIC _WIN32_WINNT=0x601  # This is to pin the log version
# )

add_subdirectory(todolist)