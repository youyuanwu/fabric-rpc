file(GLOB_RECURSE _SOURCES
  *.cpp
  *.hpp
)

# fabric rpc impl lib
add_library(fabric_rpc STATIC
  ${_SOURCES}
)

# turn on all warnings
target_compile_options(fabric_rpc PUBLIC /W4 /WX)

target_include_directories(fabric_rpc PUBLIC include)

target_link_libraries(fabric_rpc PUBLIC 
  FabricTransport 
  fabric_sdk
  fabric_internal_sdk)
