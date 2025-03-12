cmake_minimum_required(VERSION 3.12)
project(alca C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


set(ALCA_SOURCES libalca/lexer.c
        libalca/utils.c
        libalca/parser.c
        libalca/expr.c
        libalca/hashmap.c
        libalca/context.c
        libalca/module.c
        libalca/checker.c
        libalca/compiler.c
        libalca/arena.c
        libalca/bytecode.c
        libalca/vm.c
)
set(ALCA_INCLUDES "${CMAKE_SOURCE_DIR}/libalca/include"
        "${CMAKE_SOURCE_DIR}/libs/include"
)

# External libraries

## pcre2
add_subdirectory("${CMAKE_SOURCE_DIR}/libs/pcre2")
add_compile_definitions(PCRE2_CODE_UNIT_WIDTH=8)

# Libraries

## alca(-static)
add_library(alca STATIC ${ALCA_SOURCES})
target_include_directories(alca PUBLIC ${ALCA_INCLUDES})
target_compile_definitions(alca PUBLIC ALCA_BUILD_STATIC)
target_link_libraries(alca pcre2-8-static)

## alca-shared
add_library(alca-shared SHARED ${ALCA_SOURCES})
target_include_directories(alca-shared PRIVATE ${ALCA_INCLUDES})
target_compile_definitions(alca-shared PRIVATE ALCA_BUILD_DLL)
target_link_libraries(alca-shared pcre2-8-static)

# Tests
include(CTest)
add_library(testalca STATIC tests/test.c tests/test.h)
target_compile_definitions(alca PUBLIC ALCA_BUILD_DEBUG)
target_link_libraries(testalca PUBLIC alca)

# Lexer
add_executable(test_lexer tests/test_lexer.c)
target_link_libraries(test_lexer PRIVATE testalca)
add_test(NAME test_lexer COMMAND test_lexer)

# Parser
add_executable(test_parser tests/test_parser.c)
target_link_libraries(test_parser PRIVATE testalca)
add_test(NAME test_parser COMMAND test_parser WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# AST checker
add_executable(test_checker tests/test_checker.c)
target_link_libraries(test_checker PRIVATE testalca)
add_test(NAME test_checker COMMAND test_checker WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Compiler
add_executable(test_compiler tests/test_compiler.c)
target_link_libraries(test_compiler PRIVATE testalca)
add_test(NAME test_compiler COMMAND test_compiler WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# VM
add_executable(test_vm tests/test_vm.c)
target_link_libraries(test_vm PRIVATE testalca)
add_test(NAME test_vm COMMAND test_vm WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
