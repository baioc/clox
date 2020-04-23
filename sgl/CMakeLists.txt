cmake_minimum_required(VERSION 3.8)
project(SGL LANGUAGES C)

set(CMAKE_C_FLAGS "-std=c99")

set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
include(CTest)

add_library(sgl STATIC
	include/sgl/core.h
	src/core.c
	include/sgl/list.h
	src/list.c
	include/sgl/stack.h
	src/stack.c
	include/sgl/map.h
	src/map.c
	include/sgl/hash.h
	src/hash.c
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

add_executable(test_stack test/stack.c)
target_link_libraries(test_stack PUBLIC sgl)
target_include_directories(test_stack PUBLIC include)
add_test(NAME stack COMMAND test_stack)

add_executable(test_map test/map.c)
target_link_libraries(test_map PUBLIC sgl)
target_include_directories(test_map PUBLIC include)
add_test(NAME map COMMAND test_map)
