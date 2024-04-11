set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED) # need boost algo


file(GLOB_RECURSE GEN2_SOURCES
  *.cpp
  *.hpp
)

set(_exe_name fabric_rpc2_cpp_plugin)

add_executable(${_exe_name} 
  ${GEN2_SOURCES}
)

target_include_directories(${_exe_name}
  PRIVATE .
)

target_link_libraries(${_exe_name} PRIVATE libprotobuf libprotoc
  Boost::headers
)