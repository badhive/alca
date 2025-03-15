/*
 * Copyright (c) 2025 pygrum.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <alca.h>
#include <string.h>
#include <alca/arena.h>
#include <alca/bytecode.h>

#include "test.h"

void print_arr(unsigned char *arr, uint32_t size)
{
    for (int i = 0; i < size; i++)
        printf("%.2x ", arr[i]);
    printf("\n");
}

void test_compiler_module(ac_test *t)
{
    const char *path = AC_PATH_JOIN("tests", "data", "cpl_module.alca");
    ac_compiler *compiler = ac_compiler_new();
    ac_error error = ac_compiler_add_file(compiler, path);
    ac_test_assert_int32_eq(t, error, ERROR_SUCCESS);
    ac_compiler_include_module(compiler, "file", ac_test_module_file_callback);
    error = ac_compiler_compile(compiler, NULL);
    ac_test_assert_int32_eq(t, error, ERROR_SUCCESS);

    ac_arena *data = __ac_compiler_get_data(compiler);
    ac_arena *code = ac_arena_create(0);
    ac_arena_add_code_with_arg(code, OP_PUSHMODULE, ac_arena_find_string(data, "file"));
    ac_arena_add_code_with_arg(code, OP_FIELD, ac_arena_find_string(data, "name"));
    ac_arena_add_code(code, OP_OBJECT);
    ac_arena_add_code_with_arg(code, OP_PUSHSTRING, ac_arena_find_string(data, "rubeus.exe"));
    ac_arena_add_code(code, OP_STREQ);
    ac_arena_add_code_with_arg(code, OP_PUSHMODULE, ac_arena_find_string(data, "file"));
    ac_arena_add_code_with_arg(code, OP_FIELD, ac_arena_find_string(data, "size"));
    ac_arena_add_code(code, OP_OBJECT);
    ac_arena_add_code_with_arg(code, OP_PUSHINT, 2000);
    ac_arena_add_code(code, OP_GT);
    ac_arena_add_code(code, OP_ORL);

    ac_arena_add_code_with_arg(code, OP_JTRUE, ac_arena_size(code) + (5 * 3));
    ac_arena_add_code_with_arg(code, OP_PUSHBOOL, FALSE);
    ac_arena_add_code_with_arg(code, OP_JMP, ac_arena_size(code) + 5 + 17);

    ac_arena_add_code_with_arg(code, OP_PUSHMODULE, ac_arena_find_string(data, "file"));
    ac_arena_add_code_with_arg(code, OP_FIELD, ac_arena_find_string(data, "extension"));
    ac_arena_add_code(code, OP_OBJECT);
    ac_arena_add_code_with_arg(code, OP_PUSHSTRING, ac_arena_find_string(data, "exe"));
    ac_arena_add_code(code, OP_STRNE);
    ac_arena_add_code(code, OP_ANDL);
    ac_arena_add_code(code, OP_HLT);

    ac_arena *cpl_code = __ac_compiler_get_code(compiler);
    if (memcmp(ac_arena_data(code), ac_arena_data(cpl_code), ac_arena_size(code)) != 0)
        ac_test_error(t, "bytecode does not match");
}

void test_compiler_logicAnd(ac_test *t)
{
    const char *path = AC_PATH_JOIN("tests", "data", "cpl_logicAnd.alca");
    ac_compiler *compiler = ac_compiler_new();
    ac_error error = ac_compiler_add_file(compiler, path);
    ac_test_assert_int32_eq(t, error, ERROR_SUCCESS);
    error = ac_compiler_compile(compiler, NULL);
    ac_test_assert_int32_eq(t, error, ERROR_SUCCESS);

    ac_arena *code = ac_arena_create(0);
    ac_arena_add_code_with_arg(code, OP_PUSHINT, 4);
    ac_arena_add_code_with_arg(code, OP_PUSHINT, 3);
    ac_arena_add_code(code, OP_GT);

    ac_arena_add_code_with_arg(code, OP_JTRUE, 11 + (5 * 3));
    ac_arena_add_code_with_arg(code, OP_PUSHBOOL, FALSE);
    ac_arena_add_code_with_arg(code, OP_JMP, 26 + 17);

    ac_arena_add_code_with_arg(code, OP_PUSHINT, 4);
    ac_arena_add_code_with_arg(code, OP_PUSHINT, 12);
    ac_arena_add_code(code, OP_ADD);
    ac_arena_add_code_with_arg(code, OP_PUSHINT, 17);
    ac_arena_add_code(code, OP_LT);
    ac_arena_add_code(code, OP_ANDL);
    ac_arena_add_code(code, OP_HLT);

    ac_arena *cpl_code = __ac_compiler_get_code(compiler);
    if (memcmp(ac_arena_data(code), ac_arena_data(cpl_code), ac_arena_size(code)) != 0)
        ac_test_error(t, "bytecode does not match");
}

void test_compiler_arithmetic(ac_test *t)
{
    const char *path = AC_PATH_JOIN("tests", "data", "cpl_arithmetic.alca");
    ac_compiler *compiler = ac_compiler_new();
    ac_error error = ac_compiler_add_file(compiler, path);
    ac_test_assert_int32_eq(t, error, ERROR_SUCCESS);
    error = ac_compiler_compile(compiler, NULL);
    ac_test_assert_int32_eq(t, error, ERROR_SUCCESS);

    ac_arena *code = ac_arena_create(0);
    ac_arena_add_code_with_arg(code, OP_PUSHINT, 2);
    ac_arena_add_code_with_arg(code, OP_PUSHINT, 5);
    ac_arena_add_code_with_arg(code, OP_PUSHINT, 3);
    ac_arena_add_code(code, OP_MUL);
    ac_arena_add_code(code, OP_ADD);
    ac_arena_add_code_with_arg(code, OP_PUSHINT, 47);
    ac_arena_add_code(code, OP_LT);
    ac_arena_add_code(code, OP_HLT);

    ac_arena *cpl_code = __ac_compiler_get_code(compiler);
    if (memcmp(ac_arena_data(code), ac_arena_data(cpl_code), ac_arena_size(code)) != 0)
        ac_test_error(t, "bytecode does not match");
    ac_compiler_free(compiler);
}

int main()
{
    ac_tester *tester = ac_test_init();
    ac_test_add_test(tester, "compiler_arithmetic", test_compiler_arithmetic);
    ac_test_add_test(tester, "compiler_logicAnd", test_compiler_logicAnd);
    ac_test_add_test(tester, "compiler_module", test_compiler_module);
    ac_test_runall(tester);
    ac_test_destroy(tester);
}
