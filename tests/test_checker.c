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
    if (parser->error.code != AC_ERROR_SUCCESS)
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
    ac_module_table_entry file_module = ac_test_file_module();
    ac_context_add_module(ctx, &file_module);
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
    ac_module_table_entry file_module = ac_test_file_module();
    ac_context_add_module(ctx, &file_module);
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
    ac_module_table_entry file_module = ac_test_file_module();
    ac_context_add_module(ctx, &file_module);
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
    ac_module_table_entry file_module = ac_test_file_module();
    ac_context_add_module(ctx, &file_module);
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
    ac_module_table_entry file_module = ac_test_file_module();
    ac_context_add_module(ctx, &file_module);
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
    ac_module_table_entry file_module = ac_test_file_module();
    ac_context_add_module(ctx, &file_module);
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
    ac_module_table_entry file_module = ac_test_file_module();
    ac_context_add_module(ctx, &file_module);
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
    ac_module_table_entry file_module = ac_test_file_module();
    ac_context_add_module(ctx, &file_module);
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
