cmake_minimum_required(VERSION 3.13)

project(toy_isa_interpreter LANGUAGES CXX)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR
    "Threaded execution requires Clang because it uses [[clang::musttail]] (use -DCMAKE_CXX_COMPILER=clang++)"
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

add_executable(run_tests)
add_subdirectory(tests)
target_link_libraries(run_tests PRIVATE toy_isa)
