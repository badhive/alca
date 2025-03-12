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
#include <string.h>

#include <alca/checker.h>
#include <alca/context.h>
#include <alca/errors.h>
#include <alca/expr.h>
#include <alca/lexer.h>
#include <alca/parser.h>
#include <alca/types.h>
#include <alca/utils.h>

#include "test.h"

ac_ast *getAst(ac_test *t, const char *filename)
{
    ac_test_file file = {0};
    ac_test_open_file(filename, &file);
    ac_lexer *lexer = ac_lex_new(file.data, file.name, file.size);
    ac_lex_scan(lexer);
    ac_parser *parser = ac_psr_new(lexer);
    ac_ast *ast = ac_psr_parse(parser);
    if (parser->error.code != ERROR_SUCCESS)
    {
        t->test_fail = 1;
        t->error_msg = parser->error.message;
        return NULL;
    }
    ac_free(lexer);
    return ast;
}

void test_checker_check_dupRule(ac_test *t)
{
    char *filename = AC_PATH_JOIN("tests", "data", "chk_dupRule.alca");
    ac_ast *ast = getAst(t, filename);
    if (!ast) return;
    ac_context *ctx = ac_context_new();
    ac_context_add_module_load_callback(ctx, "file", ac_test_module_file_callback);
    ac_checker *checker = ac_checker_new(ast, ctx);
    if (ac_checker_check(checker))
        ac_test_error(t, "expected errors");
    int line;
    char *errormsg;
    int idx = 0;
    ac_error code;
    while (ac_checker_iter_errors(checker, &line, &code, &errormsg))
    {
        const char *expected = "tests" PSS "data" PSS "chk_dupRule.alca: error (line 11): "
        "name 'dup' already defined";
        ac_test_assert_str_eq(t, errormsg, expected)
        idx++;
    }
    ac_test_assert_int32_eq(t, idx, 1);
    ac_checker_free(checker);
    ac_context_free(ctx);
    ac_expr_free_ast(ast);
    ac_free(filename);
}

void test_checker_check_invalidSeqRule(ac_test *t)
{
    char *filename = AC_PATH_JOIN("tests", "data", "chk_invalidSeqRule.alca");
    ac_ast *ast = getAst(t, filename);
    if (!ast) return;
    ac_context *ctx = ac_context_new();
    ac_context_add_module_load_callback(ctx, "file", ac_test_module_file_callback);
    ac_checker *checker = ac_checker_new(ast, ctx);
    if (ac_checker_check(checker))
        ac_test_error(t, "expected errors");
    int line;
    char *errormsg;
    int idx = 0;
    ac_error code;
    while (ac_checker_iter_errors(checker, &line, &code, &errormsg))
    {
        const char *expected = "tests" PSS "data" PSS "chk_invalidSeqRule.alca: error (line 6): "
        "incompatible operator";
        ac_test_assert_str_eq(t, errormsg, expected)
        idx++;
    }
    ac_test_assert_int32_eq(t, idx, 1);
    ac_checker_free(checker);
    ac_context_free(ctx);
    ac_expr_free_ast(ast);
    ac_free(filename);
}

void test_checker_check_invalidSequence(ac_test *t)
{
    char *filename = AC_PATH_JOIN("tests", "data", "chk_invalidSequence.alca");
    ac_ast *ast = getAst(t, filename);
    if (!ast) return;
    ac_context *ctx = ac_context_new();
    ac_context_add_module_load_callback(ctx, "file", ac_test_module_file_callback);
    ac_checker *checker = ac_checker_new(ast, ctx);
    if (ac_checker_check(checker))
        ac_test_error(t, "expected errors");
    int line;
    char *errormsg;
    int idx = 0;
    ac_error code;
    while (ac_checker_iter_errors(checker, &line, &code, &errormsg))
    {
        const char *expected = "tests" PSS "data" PSS "chk_invalidSequence.alca: error (line 10): "
        "undefined rule 'out_scope' in sequence";
        ac_test_assert_str_eq(t, errormsg, expected)
        idx++;
    }
    ac_test_assert_int32_eq(t, idx, 1);
    ac_checker_free(checker);
    ac_context_free(ctx);
    ac_expr_free_ast(ast);
    ac_free(filename);
}

void test_checker_check_complexEval(ac_test *t)
{
    char *filename = AC_PATH_JOIN("tests", "data", "chk_complexEval.alca");
    ac_ast *ast = getAst(t, filename);
    if (!ast) return;
    ac_context *ctx = ac_context_new();
    ac_context_add_module_load_callback(ctx, "file", ac_test_module_file_callback);
    ac_checker *checker = ac_checker_new(ast, ctx);
    if (!ac_checker_check(checker))
    {
        int line;
        char *msg;
        ac_error code;
        while (ac_checker_iter_errors(checker, &line, &code, &msg)) // report first error
            ac_test_error(t, msg);
    }
    ac_checker_free(checker);
    ac_context_free(ctx);
    ac_expr_free_ast(ast);
    ac_free(filename);
}

void test_checker_check_invalidOps(ac_test *t)
{
    char *filename = AC_PATH_JOIN("tests", "data", "chk_invalidOps.alca");
    ac_ast *ast = getAst(t, filename);
    if (!ast) return;
    ac_context *ctx = ac_context_new();
    ac_context_add_module_load_callback(ctx, "file", ac_test_module_file_callback);
    ac_checker *checker = ac_checker_new(ast, ctx);
    if (ac_checker_check(checker))
        ac_test_error(t, "expected errors");
    int line;
    char *errormsg;
    int idx = 0;
    ac_error code;
    while (ac_checker_iter_errors(checker, &line, &code, &errormsg))
    {
        if (idx == 0)
        {
            const char *expected = "tests" PSS "data" PSS "chk_invalidOps.alca: error (line 4): invalid operation "
            "(type mismatch)";
            ac_test_assert_str_eq(t, errormsg, expected)
        }
        else if (idx == 1)
        {
            const char *expected = "tests" PSS "data" PSS "chk_invalidOps.alca: error (line 5): incompatible unary operator";
            ac_test_assert_str_eq(t, errormsg, expected)
        }
        else if (idx == 2)
        {
            const char *expected = "tests" PSS "data" PSS "chk_invalidOps.alca: error (line 7): incompatible operator";
            ac_test_assert_str_eq(t, errormsg, expected)
        }
        else if (idx == 3)
        {
            const char *expected = "tests" PSS "data" PSS "chk_invalidOps.alca: error (line 8): cannot use name as literal";
            ac_test_assert_str_eq(t, errormsg, expected)
        }
        idx++;
    }
    ac_test_assert_int32_eq(t, idx, 4);
    ac_checker_free(checker);
    ac_context_free(ctx);
    ac_expr_free_ast(ast);
    ac_free(filename);
}

void test_checker_check_noBool(ac_test *t)
{
    char *filename = AC_PATH_JOIN("tests", "data", "chk_noBool.alca");
    ac_ast *ast = getAst(t, filename);
    if (!ast) return;
    ac_context *ctx = ac_context_new();
    ac_context_add_module_load_callback(ctx, "file", ac_test_module_file_callback);
    ac_checker *checker = ac_checker_new(ast, ctx);
    if (ac_checker_check(checker))
        ac_test_error(t, "expected errors");
    int line;
    char *errormsg;
    int idx = 0;
    ac_error code;
    while (ac_checker_iter_errors(checker, &line, &code, &errormsg))
    {
        const char *expected = "tests" PSS "data" PSS "chk_noBool.alca: error (line 3): rule result is not boolean";
        ac_test_assert_str_eq(t, errormsg, expected)
        idx++;
    }
    ac_test_assert_int32_eq(t, idx, 1);
    ac_checker_free(checker);
    ac_context_free(ctx);
    ac_expr_free_ast(ast);
    ac_free(filename);
}

void test_checker_check_badImport(ac_test *t)
{
    char *filename = AC_PATH_JOIN("tests", "data", "chk_badImport.alca");
    ac_ast *ast = getAst(t, filename);
    if (!ast) return;
    ac_context *ctx = ac_context_new();
    ac_context_add_module_load_callback(ctx, "file", ac_test_module_file_callback);
    ac_checker *checker = ac_checker_new(ast, ctx);
    if (ac_checker_check(checker))
        ac_test_error(t, "expected errors");

    int line;
    char *errormsg;
    int idx = 0;
    ac_error code;
    while (ac_checker_iter_errors(checker, &line, &code, &errormsg))
    {
        char *expected = ac_alloc(256);
        sprintf(expected,
                "tests" PSS "data" PSS "chk_badImport.alca: error (line %d): undefined symbol 'file'",
                line);
        ac_test_assert_str_eq(t, errormsg, expected)
        ac_free(expected);
        idx++;
    }
    ac_test_assert_int32_eq(t, idx, 7);
    ac_checker_free(checker);
    ac_context_free(ctx);
    ac_expr_free_ast(ast);
    ac_free(filename);
}

void test_checker_check_validRule(ac_test *t)
{
    char *filename = AC_PATH_JOIN("tests", "data", "chk_validRule.alca");
    ac_ast *ast = getAst(t, filename);
    if (!ast) return;
    ac_context *ctx = ac_context_new();
    ac_context_add_module_load_callback(ctx, "file", ac_test_module_file_callback);
    ac_checker *checker = ac_checker_new(ast, ctx);
    if (!ac_checker_check(checker))
    {
        int line;
        char *msg;
        ac_error code;
        while (ac_checker_iter_errors(checker, &line, &code, &msg)) // report first error
            ac_test_error(t, msg);
    }
    // previous heap corruptions due to freeing data copied by hashmap
    ac_checker_free(checker);
    ac_context_free(ctx);
    ac_expr_free_ast(ast);
    ac_free(filename);
}

int main()
{
    ac_tester *tester = ac_test_init();
    ac_test_add_test(tester, "checker_check_validRule", test_checker_check_validRule);
    ac_test_add_test(tester, "checker_check_badImport", test_checker_check_badImport);
    ac_test_add_test(tester, "checker_check_noBool", test_checker_check_noBool);
    ac_test_add_test(tester, "checker_check_invalidOps", test_checker_check_invalidOps);
    ac_test_add_test(tester, "checker_check_complexEval", test_checker_check_complexEval);
    ac_test_add_test(tester, "checker_check_invalidSequence", test_checker_check_invalidSequence);
    ac_test_add_test(tester, "checker_check_invalidSequenceRule", test_checker_check_invalidSeqRule);
    ac_test_add_test(tester, "checker_check_duplicateRule", test_checker_check_dupRule);
    ac_test_runall(tester);
    ac_test_destroy(tester);
}
