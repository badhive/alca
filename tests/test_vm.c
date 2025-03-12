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

#include <alca/compiler.h>
#include <alca/vm.h>

#include "test.h"

void callback(int type, char *name, time_t at)
{
    char buffer[26] = {0};
    struct tm *tm_info = localtime(&at);
    const char *fmt = "[%s] [%s] name = \"%s\"\n";
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf(fmt, buffer, type == AC_VM_RULE ? "rule" : "sequ", name);
}

void test_vm_run_complex(ac_test *t)
{
    unsigned char fakedata[] =
        "\x00\x00\x00\x00" // version
        "\x05\x00\x00\x00" // typelen
        "file\0" // type

        "\x00\x00\x00\x00" // FILE_CREATE
        "\xFF\x00\x00\x00" // size = 255
        "exe\0" // extension = exe
        "\\Windows\\Temp\\mal.exe\0" // path
        "mal.exe\0" // name
        "\x03\x00\x00\x00" // num sections

        ".text\0" // section #1 name
        "\x0F\x00\x00\x00" // section #1 size

        ".bss\0" // section #2 name
        "\xD0\x00\x00\x00" // section #2 size

        ".rdata\0" // section #3 name
        "\x20\x00\x00\x00"; // section #3 size

    const char *path = AC_PATH_JOIN("tests", "data", "vm_complex.alca");
    ac_compiler *compiler = ac_compiler_new();
    ac_error error = ac_compiler_add_file(compiler, path);
    ac_test_assert_int32_eq(t, error, ERROR_SUCCESS);
    ac_compiler_include_module(compiler, "file", ac_test_module_file_callback);
    error = ac_compiler_compile(compiler, NULL);
    if (error != ERROR_SUCCESS)
    {
        for (int i = 0; i < compiler->error_count; i++)
            printf("ERROR %d: %s\n", compiler->errors[i].code, compiler->errors[i].message);
    }
    ac_test_assert_int32_eq(t, error, ERROR_SUCCESS);
    ac_vm *vm = ac_vm_new(compiler);
    ac_vm_add_trigger_callback(vm, callback);
    error = ac_vm_exec(vm, fakedata, sizeof(fakedata));
    ac_test_assert_int32_eq(t, error, ERROR_SUCCESS);
    ac_test_assert_int32_eq(t, ac_vm_get_trigger_count(vm), 2);
}

int main()
{
    ac_tester *tester = ac_test_init();
    ac_test_add_test(tester, "vm_run_complex", test_vm_run_complex);
    ac_test_runall(tester);
    ac_test_destroy(tester);
}
