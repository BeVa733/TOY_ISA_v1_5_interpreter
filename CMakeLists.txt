cmake_minimum_required(VERSION 3.13)

project(toy_isa_interpreter LANGUAGES CXX)

enable_testing()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  message(FATAL_ERROR
    "Threaded execution requires Clang or GCC with musttail support"
  )
endif()

add_library(toy_isa)
target_compile_features(toy_isa PUBLIC cxx_std_17)
add_subdirectory(src/toy)
target_include_directories(toy_isa PUBLIC
	include/toy
)

add_executable(interpreter)
add_subdirectory(src)
target_link_libraries(interpreter PRIVATE toy_isa)

add_subdirectory(tests)
