cmake_minimum_required(VERSION 3.13)

project(toy_isa_interpreter LANGUAGES CXX)

add_library(toy_isa)
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
