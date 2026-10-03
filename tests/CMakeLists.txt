target_sources(run_tests PRIVATE
	instruction_tests.cpp
)

add_test(NAME instruction_tests COMMAND run_tests)

add_executable(memory_tests memory_tests.cpp)
target_link_libraries(memory_tests PRIVATE toy_isa)
add_test(NAME memory_tests COMMAND memory_tests)

find_program(RUBY_EXECUTABLE ruby)
if(NOT RUBY_EXECUTABLE)
	message(FATAL_ERROR "Ruby is required to test the assembler")
endif()

add_test(
	NAME assembler_tests
	COMMAND ${RUBY_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/assembler_tests.rb
)
