find_program(RUBY_EXECUTABLE ruby)
if(NOT RUBY_EXECUTABLE)
	message(FATAL_ERROR "Ruby is required to test the assembler")
endif()

add_test(
	NAME assembler_tests
	COMMAND ${RUBY_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/assembler_tests.rb
)

add_executable(memory_tests memory_tests.cpp)
target_link_libraries(memory_tests PRIVATE toy_isa)
add_test(NAME memory_tests COMMAND memory_tests)

add_executable(decoder_tests decoder_tests.cpp)
target_link_libraries(decoder_tests PRIVATE toy_isa)
add_test(
	NAME decoder_tests
	COMMAND decoder_tests ${CMAKE_CURRENT_SOURCE_DIR}/dcoder_test/decoder.bin
)

add_executable(executor_tests executor_tests.cpp)
target_link_libraries(executor_tests PRIVATE toy_isa)
add_test(NAME executor_tests COMMAND executor_tests)
