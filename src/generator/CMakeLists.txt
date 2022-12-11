set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED) # need boost algo


set(WIN_SOURCES
 main.cpp
 gen.ixx
)

set(_exe_name fabric_rpc_cpp_plugin)

add_executable(${_exe_name} 
  ${WIN_SOURCES}
)

target_include_directories(${_exe_name}
  PRIVATE . 
  #PRIVATE ${grpc_SOURCE_DIR} ${grpc_SOURCE_DIR}/include
)

target_link_libraries(${_exe_name} PRIVATE libprotobuf libprotoc
  Boost::headers
)