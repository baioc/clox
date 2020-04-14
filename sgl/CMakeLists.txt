cmake_minimum_required(VERSION 3.8)
project(SGL LANGUAGES C)

set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
include(CTest)

add_library(sgl STATIC
	include/sgl/core.h
	src/core.c
	include/sgl/list.h
	src/list.c
)
target_include_directories(sgl PUBLIC include/sgl)

add_executable(test_core test/core.c)
target_link_libraries(test_core PUBLIC sgl)
target_include_directories(test_core PUBLIC include)
add_test(NAME core COMMAND test_core)

add_executable(test_list test/list.c)
target_link_libraries(test_list PUBLIC sgl)
target_include_directories(test_list PUBLIC include)
add_test(NAME list COMMAND test_list)
