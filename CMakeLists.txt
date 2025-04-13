cmake_minimum_required(VERSION 3.12)
project(alca C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
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
        libalca/packet.c
)

set (ALCA_MODULES libalca/modules/file.c
        libalca/modules/process.c
        libalca/modules/network.c
        libalca/modules/registry.c)

set(ALCA_INCLUDES "${CMAKE_SOURCE_DIR}/libalca/include"
        "${CMAKE_SOURCE_DIR}/libs/include"
        "${CMAKE_SOURCE_DIR}/libs/flatbuffers/include"
)

# External libraries

## pcre2
option(PCRE2_BUILD_PCRE2GREP "" OFF)
add_subdirectory("${CMAKE_SOURCE_DIR}/libs/pcre2")
add_compile_definitions(PCRE2_CODE_UNIT_WIDTH=8)

## flatbuffers
add_subdirectory("${CMAKE_SOURCE_DIR}/libs/flatbuffers")

set(ALCA_CLI_INCLUDES "${CMAKE_SOURCE_DIR}/libs/CLI11/include"
        "${CMAKE_SOURCE_DIR}/libs/flatbuffers/include"
)

set(ALCA_LINK_LIBRARIES pcre2-8-static)

## alca-static
add_library(alca-static STATIC ${ALCA_SOURCES} ${ALCA_MODULES})
target_include_directories(alca-static PUBLIC ${ALCA_INCLUDES})
target_compile_definitions(alca-static PUBLIC ALCA_BUILD_STATIC "ALCA_BUILD_DEBUG=$<CONFIG:Debug>")
target_link_libraries(alca-static ${ALCA_LINK_LIBRARIES})

## alca-shared
add_library(alca-shared SHARED ${ALCA_SOURCES} ${ALCA_MODULES})
target_include_directories(alca-shared PRIVATE ${ALCA_INCLUDES})
target_compile_definitions(alca-shared PRIVATE ALCA_BUILD_DLL "ALCA_BUILD_DEBUG=$<CONFIG:Debug>")
target_link_libraries(alca-shared ${ALCA_LINK_LIBRARIES})

## alca cli
add_executable(alca cli/alca.cpp
        cli/conn.cpp
        cli/conn.hpp
        cli/modules.cpp
        cli/modules.hpp
        cli/utils.cpp
        cli/alca.fb.hpp
)

target_include_directories(alca PRIVATE ${ALCA_CLI_INCLUDES})
target_link_libraries(alca PRIVATE alca-static flatbuffers)
if(WIN32)
    target_link_libraries(alca PRIVATE wsock32 ws2_32 "-static")
endif()

# Tests
include(CTest)
add_library(testalca STATIC tests/test.c tests/test.h)
target_link_libraries(testalca PUBLIC alca-static)

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
