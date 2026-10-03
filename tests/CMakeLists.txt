target_sources(run_tests PRIVATE
	instruction_tests.cpp
)

add_test(NAME instruction_tests COMMAND run_tests)

find_program(RUBY_EXECUTABLE ruby)
if(NOT RUBY_EXECUTABLE)
	message(FATAL_ERROR "Ruby is required to test the assembler")
endif()

add_test(
	NAME assembler_tests
	COMMAND ${RUBY_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/assembler_tests.rb
)
