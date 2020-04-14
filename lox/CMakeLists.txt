cmake_minimum_required(VERSION 3.8)
project(Lox LANGUAGES C)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

add_executable(lox apps/main.c)
target_link_libraries(lox PRIVATE sgl)
target_include_directories(lox PRIVATE ${CMAKE_SOURCE_DIR}/sgl/include)

add_library(clox
	include/clox/common.h
	src/common.c
	include/clox/chunk.h
	src/chunk.c
)
target_include_directories(clox PUBLIC include/clox)
