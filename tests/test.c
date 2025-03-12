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

#include <stdio.h>
#include <stdlib.h>

#include "test.h"

#include <string.h>
#include <alca/module.h>

typedef struct test_unmarshal_t
{
    const unsigned char *data;
    uint32_t index;
} test_unmarshal_t;

/** Runs all registered tests.
 *
 * @param tester testing object
 * @return non-zero value if any of the tests failed
 */
int ac_test_runall(ac_tester *tester)
{
    int pass_count = 0;
    for (ac_test *test = tester->tests; test; test = test->next)
    {
        test->test_function(test);
        if (test->test_fail)
        {
            tester->any_test_fail = 1;
            fprintf(stderr, "[FAIL] TEST #%d = %s", test->pos, test->name);
            if (test->error_msg)
            {
                fprintf(stderr, ": error: %s", test->error_msg);
            }
            fprintf(stderr, "\n");
        } else
        {
            printf("[PASS] TEST #%d = %s\n", test->pos, test->name);
            pass_count++;
        }
        fflush(stdout);
        fflush(stderr);
    }
    printf("%d of %d sub-tests passed (%d%%)\n", pass_count, tester->test_count, pass_count * 100 / tester->test_count);
    return tester->any_test_fail;
}

/** Registers a new test.
 *
 * @param tester testing object
 * @param test_name name of test to run
 * @param test_function function to run as part of the test
 */
void ac_test_add_test(ac_tester *tester, const char *test_name, ac_test_function test_function)
{
    ac_test *t = NULL;
    ac_test *test = calloc(1, sizeof(ac_test));
    if (test == NULL)
    {
        perror("failed to allocate memory for test");
        return;
    }
    tester->test_count++;
    test->name = test_name;
    test->pos = tester->test_count;
    test->test_function = test_function;
    if (!tester->tests)
        tester->tests = test;
    else
    {
        for (t = tester->tests; t->next; t = t->next)
        {
        }
        t->next = test;
    }
}

/** Initialises the testing object.
 *
 * @return testing object
 */
ac_tester *ac_test_init()
{
    ac_tester *tester = calloc(1, sizeof(ac_tester));
    if (tester == NULL)
    {
        perror("failed to allocate memory for tester");
        return NULL;
    }
    return tester;
}

void ac_test_open_file(const char *path, ac_test_file *testFile)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        perror("Error opening rule file");
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(size+1);
    if (buffer == NULL)
    {
        perror("Error allocating memory");
        fclose(file);
        exit(1);
    }
    buffer[size] = '\0';
    fread(buffer, 1, size, file);
    fclose(file);
    if (testFile)
    {
        testFile->name = path;
        testFile->size = size;
        testFile->data = buffer;
    }
    else
        free(buffer);
}

void ac_test_free_file(ac_test_file *testFile)
{
    if (testFile)
    {
        if (testFile->data)
            free(testFile->data);
    }
}

char *test_unmarshal_string(test_unmarshal_t *um)
{
    char *str = (char *)(um->data + um->index);
    um->index += strlen(str) + 1;
    return str;
}

uint32_t test_unmarshal_uint32(test_unmarshal_t *um)
{
    uint32_t result = *(uint32_t *)(um->data + um->index);
    um->index += sizeof(uint32_t);
    return result;
}

int ac_test_module_file_unmarshal(ac_module *module, const unsigned char *edata)
{
    test_unmarshal_t um = {.index = 0, .data = edata};
    ac_module *sections = ac_module_get_field(module, "sections");
    ac_module_set_uint32_field(module, "action", test_unmarshal_uint32(&um));
    ac_module_set_uint32_field(module, "size", test_unmarshal_uint32(&um));
    ac_module_set_string_field(module, "extension", test_unmarshal_string(&um));
    ac_module_set_string_field(module, "path", test_unmarshal_string(&um));
    ac_module_set_string_field(module, "name", test_unmarshal_string(&um));
    uint32_t len = test_unmarshal_uint32(&um);
    ac_module_set_uint32_field(module, "num_sections", len);
    for (int i = 0; i < len; i++)
    {
        char *name = test_unmarshal_string(&um);
        uint32_t size = test_unmarshal_uint32(&um);
        ac_module *obj = ac_module_create_item_for_struct_array(sections);
        ac_module_set_string_field(obj, "name", name);
        ac_module_set_uint32_field(obj, "size", size);
        ac_object o = {.o = obj};
        ac_module_array_field_append(sections, AC_FIELD_TYPE_STRUCT, &o);
    }
    return TRUE;
}

ac_module *ac_test_module_file_callback()
{
    enum file_event_action
    {
        FILE_CREATE,
        FILE_DELETE,
        FILE_MODIFY,
        FILE_RENAME,
    };
    ac_module *module = ac_module_create("file", AC_VERSION(0, 0, 0), NULL);
    ac_module_set_unmarshaller(module, ac_test_module_file_unmarshal);
    ac_module_add_field(module, "action", AC_FIELD_TYPE_INTEGER);
    ac_module_add_field(module, "size", AC_FIELD_TYPE_INTEGER);
    ac_module_add_field(module, "extension", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "path", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "name", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "num_sections", AC_FIELD_TYPE_INTEGER);

    ac_module *section_list = ac_module_add_field(module, "sections", AC_FIELD_TYPE_ARRAY | AC_FIELD_TYPE_STRUCT);
    ac_module_add_field(section_list, "name", AC_FIELD_TYPE_STRING);
    ac_module_add_field(section_list, "size", AC_FIELD_TYPE_INTEGER);
    ac_module_add_enum(module, FILE_CREATE);
    ac_module_add_enum(module, FILE_DELETE);
    ac_module_add_enum(module, FILE_MODIFY);
    ac_module_add_enum(module, FILE_RENAME);
    return module;
}
