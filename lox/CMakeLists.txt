cmake_minimum_required(VERSION 3.8)
project(Lox LANGUAGES C)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

include_directories(${CMAKE_SOURCE_DIR}/sgl/include)

add_executable(lox apps/main.c)
target_link_libraries(lox PUBLIC clox)
target_include_directories(lox PUBLIC include)

add_library(clox
	include/clox/common.h
	src/common.c
	include/clox/chunk.h
	src/chunk.c
	include/clox/debug.h
	src/debug.c
	include/clox/value.h
	src/value.c
	include/clox/vm.h
	src/vm.c
	include/clox/compiler.h
	src/compiler.c
	include/clox/scanner.h
	src/scanner.c
	include/clox/object.h
	src/object.c
	include/clox/memory.h
	src/memory.c
)
target_include_directories(clox PUBLIC include/clox)
target_link_libraries(lox PRIVATE sgl)
