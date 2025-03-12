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

#ifndef AC_TEST_H
#define AC_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <alca/module.h>

typedef unsigned char bool;
typedef struct _ac_test ac_test;

typedef void (*ac_test_function)(ac_test *);

typedef struct _ac_tester ac_tester;
typedef struct _ac_test_file ac_test_file;

#ifdef _WIN32
#define PSS "\\"
#else
#define PSS "/"
#endif

/** Marks a test as failed with and exits immediately with an optional error message.
 *
 * @param test test object passed by tester
 * @param message error message, optional. Must be allocated with malloc, as it is freed by the tester on exit.
 */
#define ac_test_error(test, message) {\
    if (test) { \
        test->test_fail = 1; \
        test->error_msg = message; \
    } \
    return; }

#define ac_test_assert_str_eq(test, got, expected) {\
    if (test) { \
        if (!(got == NULL && expected == NULL)) { \
            if (strncmp(expected, got, strlen(expected)) != 0) { \
                char* msg = malloc(1024); \
                snprintf(msg, 1024, "expected string '%s', got '%s'", (char *)expected, (char *)got); \
                ac_test_error(test, msg); \
            } \
        } \
    }}

#define ac_test_assert_bool_eq(test, got, expected) {\
    if (test) { \
        if (got != expected) { \
            char* msg = malloc(1024); \
            snprintf(msg, 1024, "expected '%s', got '%s'", expected ? "true" : "false", got ? "true" : "false"); \
            ac_test_error(test, msg); \
        } \
    }}

#define ac_test_assert_int32_eq(test, got, expected) {\
    if (test) { \
        if (sizeof(got) != 4 || expected != got) { \
            char* msg = malloc(256); \
            snprintf(msg, 256, "expected 32-bit integer '%d', got '%d'", expected, got); \
            ac_test_error(test, msg); \
        } \
    }}

#define ac_test_assert_int64_eq(test, got, expected) {\
    if (test) { \
        if (sizeof(got) != 8 || expected != got) { \
            char* msg = malloc(256); \
            snprintf(msg, 256, "expected 64-bit integer '%lld', got '%lld'", expected, got); \
            ac_test_error(test, msg); \
        } \
    }}

/** Decommissions the testing object and exits the test program. Should be called in the main function
 * as the final line of code.
 *
 * @param tester initialised testing object
 */
#define ac_test_destroy(tester) {\
    int __ac_test_retval = 0; \
    if (tester) { \
        __ac_test_retval = tester->any_test_fail; \
        ac_test* t = tester->tests; \
        ac_test* old = NULL; \
        while (t) { \
            old = t; \
            t = t->next; \
            free(old); \
        } \
        free(tester); \
    } \
    return __ac_test_retval; }

struct _ac_test
{
    const char *name;
    int pos;
    char *error_msg;
    bool free_msg;
    bool test_fail;
    ac_test_function test_function;
    struct _ac_test *next;
};

struct _ac_tester
{
    ac_test *tests;
    int test_count;
    bool any_test_fail;
};

struct _ac_test_file
{
    const char *name;
    size_t size;
    char *data;
};

ac_module *ac_test_module_file_callback();

void ac_test_open_file(const char *path, ac_test_file *testFile);

void ac_test_free_file(ac_test_file *testFile);

int ac_test_runall(ac_tester *tester);

void ac_test_add_test(ac_tester *tester, const char *test_name, ac_test_function test_function);

ac_tester *ac_test_init();

#endif //AC_TEST_H
