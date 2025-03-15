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

#include <alca/compiler.h>
#include <alca/vm.h>

#include "test.h"

void callback(int type, char *name, time_t at, void *ectx)
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
    error = ac_vm_exec(vm, fakedata, sizeof(fakedata), NULL);
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
