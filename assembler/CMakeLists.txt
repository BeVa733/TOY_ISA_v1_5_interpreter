add_executable(toy_elf_writer elf_writer.cpp)
target_compile_features(toy_elf_writer PRIVATE cxx_std_17)
target_include_directories(toy_elf_writer PRIVATE
	${ELFIO_INCLUDE_DIRECTORY}
	${CMAKE_CURRENT_SOURCE_DIR}/../include
)

set_target_properties(toy_elf_writer PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
