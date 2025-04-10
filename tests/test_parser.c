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

#include <alca/errors.h>
#include <alca/expr.h>
#include <alca/lexer.h>
#include <alca/parser.h>
#include <alca/types.h>
#include <alca/utils.h>

#include "test.h"

void traverseExpression(ac_test *t, ac_expr *actual, ac_expr *expected)
{
    if (!expected && actual)
    {
        ac_test_error(t, "expected null expression, got value");
    }
    if (expected && !actual)
    {
        ac_test_error(t, "expected expression, got null");
    }
    if (expected)
    {
        ac_test_assert_int32_eq(t, actual->type, expected->type);
        switch (expected->type)
        {
            case EXPR_BINARY:
                ac_test_assert_int32_eq(t, actual->u.binary.op->type, expected->u.binary.op->type);
                traverseExpression(t, actual->u.binary.left, expected->u.binary.left);
                traverseExpression(t, actual->u.binary.right, expected->u.binary.right);
                break;
            case EXPR_UNARY:
                ac_test_assert_int32_eq(t, actual->u.unary.op->type, expected->u.unary.op->type);
                traverseExpression(t, actual->u.unary.right, expected->u.unary.right);
                break;
            case EXPR_LITERAL:
                ac_test_assert_int32_eq(t, actual->u.literal.value->type, expected->u.literal.value->type);
                switch (expected->u.literal.value->type)
                {
                    case TOKEN_IDENTIFIER:
                    case TOKEN_STRING:
                        ac_test_assert_str_eq(t, actual->u.literal.value->value, expected->u.literal.value->value);
                        break;
                    case TOKEN_NUMBER:
                        if (!actual->u.literal.value->value)
                        {
                            ac_test_error(t, "got no numerical literal value");
                        }
                        ac_test_assert_int32_eq(t,
                                                *(uint32_t*)actual->u.literal.value->value,
                                                *(uint32_t*)expected->u.literal.value->value);
                    default:
                        break;
                }
                break;
            case EXPR_GROUPING:
                traverseExpression(t, actual->u.grouping.expression, expected->u.grouping.expression);
                break;
            case EXPR_CALL:
                ac_test_assert_int32_eq(t, actual->u.call.arg_count, expected->u.call.arg_count);
                traverseExpression(t, actual->u.call.callee, expected->u.call.callee);
                for (int i = 0; i < actual->u.call.arg_count; i++)
                    traverseExpression(t, actual->u.call.arguments[i], expected->u.call.arguments[i]);
                break;
            case EXPR_FIELD:
                ac_test_assert_int32_eq(t, actual->u.field.field_name->type, expected->u.field.field_name->type);
                ac_test_assert_str_eq(t, actual->u.field.field_name->value, expected->u.field.field_name->value);
                traverseExpression(t, actual->u.field.object, expected->u.field.object);
                break;
            case EXPR_INDEX:
                traverseExpression(t, actual->u.index.object, expected->u.index.object);
                traverseExpression(t, actual->u.index.index, expected->u.index.index);
                break;
            default:
                break;
        }
    }
}

void traverseRule(ac_test *t, ac_statement *actual, ac_statement *expected)
{
    ac_test_assert_bool_eq(t, actual->u.rule.external, expected->u.rule.external);
    if (actual->u.rule.external)
        ac_test_assert_str_eq(t, actual->u.rule.name->value, expected->u.rule.name->value);
    traverseExpression(t, actual->u.rule.condition, expected->u.rule.condition);
}

void traverseSequence(ac_test *t, ac_statement *actual, ac_statement *expected)
{
    ac_test_assert_str_eq(t, actual->u.sequence.name->value, expected->u.sequence.name->value);
    ac_test_assert_int32_eq(t, actual->u.sequence.rule_count, expected->u.sequence.rule_count);
    ac_test_assert_int32_eq(t, actual->u.sequence.max_span, expected->u.sequence.max_span);
    if (actual->u.sequence.rule_count)
    {
        for (uint32_t i = 0; i < actual->u.sequence.rule_count; i++)
            traverseRule(t, actual->u.sequence.rules[i], expected->u.sequence.rules[i]);
    }
}

void traverseImport(ac_test *t, ac_statement *actual, ac_statement *expected)
{
    ac_test_assert_str_eq(t, actual->u.import.name->value, expected->u.import.name->value);
}

void validate(ac_test *t, ac_ast *actual, ac_ast *expected)
{
    ac_test_assert_str_eq(t, actual->path, expected->path);
    ac_test_assert_int32_eq(t, actual->stmt_count, expected->stmt_count);
    if (actual->stmt_count)
    {
        for (uint32_t i = 0; i < actual->stmt_count; i++)
        {
            if (actual->statements[i]->type == STMT_SEQUENCE)
                traverseSequence(t, actual->statements[i], expected->statements[i]);
            else if (actual->statements[i]->type == STMT_RULE)
                traverseRule(t, actual->statements[i], expected->statements[i]);
            else if (actual->statements[i]->type == STMT_IMPORT)
                traverseImport(t, actual->statements[i], expected->statements[i]);
        }
    }
}

ac_lexer *getFileLexer(const char *filename)
{
    ac_test_file file = {0};
    ac_test_open_file(filename, &file);
    ac_lexer *lexer = ac_lex_new(file.data, file.name, file.size);
    ac_lex_scan(lexer);
    return lexer;
}

ac_expr *literal(ac_token_type type, void *value)
{
    ac_token *token = malloc(sizeof(ac_token));
    token->type = type;
    token->value = value;
    return ac_expr_new_literal(token);
}

ac_expr *field(char *name, ac_expr *object)
{
    ac_token *token = malloc(sizeof(ac_token));
    token->type = TOKEN_IDENTIFIER;
    token->value = name;
    return ac_expr_new_field(object, token);
}

ac_statement *import(char *name)
{
    ac_token *token = malloc(sizeof(ac_token));
    token->type = TOKEN_IDENTIFIER;
    token->value = name;
    return ac_expr_new_import(token);
}

void test_psr_parse_indexCall(ac_test *t)
{
    ac_ast pr = {0};
    pr.stmt_count = 2;

    ac_statement rule = {0};
    ac_token rule_name = {.type = TOKEN_IDENTIFIER, .value = "check_section_hash"};
    rule.u.rule.name = &rule_name;

    ac_expr *fileGlobal = literal(TOKEN_IDENTIFIER, "file");
    ac_expr *sectionsFld = field("sections", fileGlobal);
    int zero = 0;
    ac_token bracket = {.type = TOKEN_RBRACKET, .line = 2};
    ac_expr *zeroIdx = literal(TOKEN_NUMBER, &zero);
    ac_expr *sectionIdx = ac_expr_new_index(sectionsFld, zeroIdx, &bracket);
    ac_expr *idxFld = field("crc32", sectionIdx);
    ac_expr *crc32 = ac_expr_new_call(idxFld, NULL);
    int hundo = 100;
    ac_expr *hundred = literal(TOKEN_NUMBER, &hundo);
    ac_expr_call_append_argument(crc32, zeroIdx);
    ac_expr_call_append_argument(crc32, hundred);

    int deadb = 0xdeadbeef;
    ac_expr *deadbeef = literal(TOKEN_NUMBER, &deadb);
    ac_token eqOp = {.type = TOKEN_EQUAL_EQUAL};

    ac_expr *and = ac_expr_new_binary(crc32, &eqOp, deadbeef);

    rule.u.rule.condition = and;
    ac_statement *prule = &rule;
    ac_statement *stmts[] = {import("file"), prule};
    pr.statements = stmts;

    char *filename = AC_PATH_JOIN("tests", "data", "psr_indexCall.alca");
    pr.path = filename;

    ac_lexer *lexer = getFileLexer(filename);
    ac_parser *parser = ac_psr_new(lexer);
    ac_ast *program = ac_psr_parse(parser);
    if (ac_psr_get_last_error(parser) != AC_ERROR_SUCCESS)
    {
        ac_test_error(t, ac_psr_get_last_error_message(parser));
    }
    validate(t, program, &pr);
}

void test_psr_parse_complexRule(ac_test *t)
{
    ac_ast pr = {0};
    pr.stmt_count = 3;

    ac_statement rule = {.type = STMT_RULE};
    ac_token rule_name = {.type = TOKEN_IDENTIFIER, .value = "detect_dropper"};
    rule.u.rule.name = &rule_name;

    ac_expr *fileGlobal = literal(TOKEN_IDENTIFIER, "file");
    ac_expr *actionFld = field("action", fileGlobal);
    ac_expr *extFld = field("extension", fileGlobal);
    ac_expr *pathFld = field("path", fileGlobal);
    ac_expr *enumFld = field("FILE_CREATE", fileGlobal);

    ac_expr *procGlobal = literal(TOKEN_IDENTIFIER, "process");
    ac_expr *nameFld = field("name", procGlobal);

    ac_expr *exeStr = literal(TOKEN_STRING, "exe");
    ac_expr *tmpStr = literal(TOKEN_STRING, "C:\\Windows\\temp");
    ac_expr *dropStr = literal(TOKEN_STRING, "dropper.exe");

    ac_token andOp = {.type = TOKEN_AND};
    ac_token eqOp = {.type = TOKEN_EQUAL_EQUAL};
    ac_token notOp = {.type = TOKEN_NOT};
    ac_expr *bin1 = ac_expr_new_binary(nameFld, &eqOp, dropStr);
    ac_expr *bin2 = ac_expr_new_binary(pathFld, &eqOp, tmpStr);
    ac_expr *una1 = ac_expr_new_unary(&notOp, bin2);
    ac_expr *bin3 = ac_expr_new_binary(extFld, &eqOp, exeStr);
    ac_expr *bin4 = ac_expr_new_binary(actionFld, &eqOp, enumFld);

    ac_expr *and1 = ac_expr_new_binary(bin4, &andOp, bin3);
    ac_expr *and2 = ac_expr_new_binary(and1, &andOp, una1);
    ac_expr *and3 = ac_expr_new_binary(and2, &andOp, bin1);

    rule.u.rule.condition = and3;
    ac_statement *prule = &rule;
    ac_statement *stmts[] = {import("file"), import("process"), prule};
    pr.statements = stmts;

    char *filename = AC_PATH_JOIN("tests", "data", "psr_complexRule.alca");
    pr.path = filename;

    ac_lexer *lexer = getFileLexer(filename);
    ac_parser *parser = ac_psr_new(lexer);
    ac_ast *program = ac_psr_parse(parser);
    if (ac_psr_get_last_error(parser) != AC_ERROR_SUCCESS)
    {
        ac_test_error(t, ac_psr_get_last_error_message(parser));
    }
    validate(t, program, &pr);
}

void test_psr_parse_simpleRule(ac_test *t)
{
    ac_ast pr = {0};
    pr.stmt_count = 3;

    // initialise rule
    ac_statement rule = {.type =  STMT_RULE};
    ac_token rule_name = {.type = TOKEN_IDENTIFIER, .value = "test_rule"};
    rule.u.rule.name = &rule_name;

    ac_expr *global = literal(TOKEN_IDENTIFIER, "file");
    ac_expr *fld = field("name", global);
    ac_expr *fn = literal(TOKEN_STRING, "Rubeus.exe");

    ac_token op = {.type = TOKEN_EQUAL_EQUAL};
    ac_expr cond = {.type = EXPR_BINARY, .u.binary.left = fld, .u.binary.op = &op, .u.binary.right = fn};
    rule.u.rule.condition = &cond;

    // initialise sequence
    ac_statement sequence = {0};
    ac_token sequence_name = {.type = TOKEN_IDENTIFIER, .value = "test_sequence"};
    sequence.u.sequence.name = &sequence_name;
    sequence.u.sequence.max_span = 5 * 60;
    sequence.u.sequence.rule_count = 2;

    ac_statement rule2 = {.type = STMT_RULE};
    rule2.u.rule.name = &rule_name;
    rule2.u.rule.external = TRUE;
    ac_statement rule3 = {.type = STMT_RULE};
    rule3.u.rule.condition = &cond;

    ac_statement *rules[] = {&rule2, &rule3};
    sequence.u.sequence.rules = rules;

    ac_statement *prule = &rule;
    ac_statement *pseq = &sequence;

    ac_statement *stmts[] = {import("file"), prule, pseq};
    pr.statements = stmts;

    char *filename = AC_PATH_JOIN("tests", "data", "psr_simpleRule.alca");
    pr.path = filename;

    ac_lexer *lexer = getFileLexer(filename);
    ac_parser *parser = ac_psr_new(lexer);
    ac_ast *program = ac_psr_parse(parser);
    if (ac_psr_get_last_error(parser) != AC_ERROR_SUCCESS)
    {
        ac_test_error(t, ac_psr_get_last_error_message(parser));
    }
    validate(t, program, &pr);
}

int main()
{
    ac_tester *tester = ac_test_init();
    ac_test_add_test(tester, "psr_parse_simpleRule", test_psr_parse_simpleRule);
    ac_test_add_test(tester, "psr_parse_complexRule", test_psr_parse_complexRule);
    ac_test_add_test(tester, "psr_parse_indexCall", test_psr_parse_indexCall);
    ac_test_runall(tester);
    ac_test_destroy(tester);
}
