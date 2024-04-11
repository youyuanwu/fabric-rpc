set (CMAKE_CXX_STANDARD 20)

set(Boost_USE_STATIC_LIBS ON) # use static boost
find_package(Boost REQUIRED COMPONENTS unit_test_framework )


add_compile_definitions(
  _WIN32_WINNT=0x0601
)

add_subdirectory(base_test)
add_subdirectory(todolist)
add_subdirectory(helloworld_bench)

add_subdirectory(base2_test)