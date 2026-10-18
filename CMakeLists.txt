cmake_minimum_required(VERSION 3.13)

project(toy_isa_interpreter LANGUAGES CXX)

set(IR_SOURCE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ir")
if(NOT EXISTS "${IR_SOURCE_DIRECTORY}/ir.h")
	message(FATAL_ERROR
		"The ir submodule is missing. Run "
		"git submodule update --init --recursive"
	)
endif()

set(IR_LIBRARY "${IR_SOURCE_DIRECTORY}/libir.a")

add_custom_target(ir_build
	COMMAND make
	WORKING_DIRECTORY "${IR_SOURCE_DIRECTORY}"
	BYPRODUCTS "${IR_LIBRARY}"
	COMMENT "Building IR"
)

add_library(ir INTERFACE)
add_dependencies(ir ir_build)
target_include_directories(ir INTERFACE "${IR_SOURCE_DIRECTORY}")
target_link_libraries(ir INTERFACE
	"${IR_LIBRARY}"
	${CMAKE_DL_LIBS}
	m
)

find_path(ELFIO_INCLUDE_DIRECTORY elfio/elfio.hpp)
if(NOT ELFIO_INCLUDE_DIRECTORY)
	message(FATAL_ERROR "ELFIO is required for the project")
endif()

enable_testing()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  message(FATAL_ERROR
    "Threaded execution requires Clang or GCC with musttail support"
  )
endif()

add_library(toy_isa)
target_compile_features(toy_isa PUBLIC cxx_std_17)
target_link_libraries(toy_isa PRIVATE ir)
add_subdirectory(src/toy)
add_subdirectory(src/execution_type)
target_include_directories(toy_isa PUBLIC
	include/toy
)

add_executable(interpreter)
add_subdirectory(src)
target_link_libraries(interpreter PRIVATE toy_isa)

add_subdirectory(assembler)

add_subdirectory(benchmark)

add_subdirectory(tests)
