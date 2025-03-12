/*
Copyright (c) 2025, pygrum. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
