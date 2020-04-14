cmake_minimum_required(VERSION 3.8)
project(Lox LANGUAGES C)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

include_directories(${CMAKE_SOURCE_DIR}/sgl/include)

add_executable(lox apps/main.c)
target_link_libraries(lox PUBLIC clox)

add_library(clox
	include/clox/common.h
	src/common.c
	include/clox/chunk.h
	src/chunk.c
	include/clox/debug.h
	src/debug.c
)
target_include_directories(clox PUBLIC include/clox)
target_link_libraries(lox PRIVATE sgl)
