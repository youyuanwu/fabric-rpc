set (CMAKE_CXX_STANDARD 20) # use semaphore

file(GLOB tool_sources
  *.cpp
  *.hpp
)

add_library(fabric_rpc_tool STATIC
  ${tool_sources}
)

target_include_directories(fabric_rpc_tool PUBLIC
  ./include
)

target_link_libraries(fabric_rpc_tool PUBLIC
  fabric_internal_headers
)

