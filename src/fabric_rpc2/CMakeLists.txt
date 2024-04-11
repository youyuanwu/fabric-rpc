# requires asio
find_package(Boost REQUIRED COMPONENTS)

set (CMAKE_CXX_STANDARD 20)

file(GLOB _SOURCES
  include/fabricrpc/*.hpp
  src/*.cpp
)


set(_lib_name fabric_rpc2)
add_library(${_lib_name} STATIC
 ${_SOURCES}
)

target_link_libraries(${_lib_name}
  PUBLIC Boost::headers
  PUBLIC FabricTransport 
  fabric_sdk
  fabric_internal_sdk
  fabric_rpc_tool
  fabric_rpc_proto
  absl::status
  absl::strings
)

target_compile_definitions(${_lib_name}
INTERFACE WIN32_LEAN_AND_MEAN # This is to get rid of include from fabric of winsock.h for asio
)

target_include_directories(${_lib_name}
  PUBLIC include
)