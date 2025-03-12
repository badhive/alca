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

#include <alca/utils.h>

#include <alca/expr.h>

ac_expr *ac_expr_new_binary(ac_expr *left, ac_token *op, ac_expr *right)
{
    ac_expr *expr = ac_alloc(sizeof(ac_expr));
    expr->type = EXPR_BINARY;
    expr->u.binary.left = left;
    expr->u.binary.operator = op;
    expr->u.binary.right = right;
    return expr;
}

ac_expr *ac_expr_new_unary(ac_token *op, ac_expr *right)
{
    ac_expr *expr = ac_alloc(sizeof(ac_expr));
    expr->type = EXPR_UNARY;
    expr->u.unary.operator = op;
    expr->u.unary.right = right;
    return expr;
}

ac_expr *ac_expr_new_grouping(ac_expr *expression)
{
    ac_expr *expr = ac_alloc(sizeof(ac_expr));
    expr->type = EXPR_GROUPING;
    expr->u.grouping.expression = expression;
    return expr;
}

ac_expr *ac_expr_new_literal(ac_token *value)
{
    ac_expr *expr = ac_alloc(sizeof(ac_expr));
    expr->type = EXPR_LITERAL;
    expr->u.literal.value = value;
    return expr;
}

ac_expr *ac_expr_new_call(ac_expr *callee, ac_token *paren)
{
    ac_expr *expr = ac_alloc(sizeof(ac_expr));
    expr->type = EXPR_CALL;
    expr->u.call.callee = callee;
    expr->u.call.paren = paren;
    return expr;
}

void ac_expr_call_append_argument(ac_expr *call, ac_expr *argument)
{
    if (call)
    {
        if (!call->u.call.arguments)
            call->u.call.arguments = ac_alloc(sizeof(ac_expr *));
        else
            call->u.call.arguments = ac_realloc(call->u.call.arguments,
                                                sizeof(ac_expr *) * (call->u.call.arg_count + 1));
        call->u.call.arguments[call->u.call.arg_count++] = argument;
    }
}

ac_expr *ac_expr_new_field(ac_expr *object, ac_token *field_name)
{
    ac_expr *expr = ac_alloc(sizeof(ac_expr));
    expr->type = EXPR_FIELD;
    expr->u.field.object = object;
    expr->u.field.field_name = field_name;
    return expr;
}

ac_expr *ac_expr_new_index(ac_expr *object, ac_expr *index, ac_token *bracket)
{
    ac_expr *expr = ac_alloc(sizeof(ac_expr));
    expr->type = EXPR_INDEX;
    expr->u.index.object = object;
    expr->u.index.index = index;
    expr->u.index.bracket = bracket;
    return expr;
}

ac_expr *ac_expr_new_range(uint32_t match_type, uint32_t fixed, ac_token *ivar, ac_expr *start, ac_expr *end, ac_expr *cond)
{
    ac_expr *expr = ac_alloc(sizeof(ac_expr));
    expr->type = EXPR_RANGE;
    switch (match_type)
    {
        case AC_RANGE_MATCH_ANY: expr->u.range.any = TRUE; break;
        case AC_RANGE_MATCH_ALL: expr->u.range.all = TRUE; break;
        case AC_RANGE_MATCH_FIXED: expr->u.range.fixed = fixed; break;
        default: expr->u.range.all = TRUE; break;
    }
    expr->u.range.ivar = ivar;
    expr->u.range.start = start;
    expr->u.range.end = end;
    expr->u.range.condition = cond;
    return expr;
}

ac_statement *ac_expr_new_rule(ac_token *name, ac_token *event, ac_expr *condition, int external, int private)
{
    ac_statement *rule = ac_alloc(sizeof(ac_statement));
    rule->type = STMT_RULE;
    rule->u.rule.name = name;
    rule->u.rule.event = event;
    rule->u.rule.condition = condition;
    rule->u.rule.external = external;
    rule->u.rule.private = private;
    return rule;
}

ac_statement *ac_expr_new_sequence(ac_token *name, uint32_t maxSpan)
{
    ac_statement *seq = ac_alloc(sizeof(ac_statement));
    seq->type = STMT_SEQUENCE;
    seq->u.sequence.name = name;
    seq->u.sequence.max_span = maxSpan;
    return seq;
}

ac_statement *ac_expr_new_import(ac_token *name)
{
    ac_statement *import = ac_alloc(sizeof(ac_statement));
    import->type = STMT_IMPORT;
    import->u.import.name = name;
    return import;
}

void ac_expr_sequence_append_rule(ac_statement *seq, ac_statement *rule)
{
    if (seq)
    {
        if (!seq->u.sequence.rules)
            seq->u.sequence.rules = ac_alloc(sizeof(ac_statement *));
        else
            seq->u.sequence.rules = ac_realloc(seq->u.sequence.rules, sizeof(ac_statement *) * (seq->u.sequence.rule_count + 1));
        seq->u.sequence.rules[seq->u.sequence.rule_count++] = rule;
    }
}

ac_ast *ac_expr_new_ast(const char *path)
{
    ac_ast *program = ac_alloc(sizeof(ac_ast));
    program->path = path;
    return program;
}

void ac_expr_ast_add_stmt(ac_ast *program, ac_statement *stmt)
{
    if (program)
    {
        if (!program->statements)
            program->statements = ac_alloc(sizeof(ac_statement *));
        else
            program->statements = ac_realloc(program->statements, sizeof(ac_statement *) * (program->stmt_count + 1));
        program->statements[program->stmt_count++] = stmt;
    }
}

// frees expressions but not tokens
void ac_expr_free_expression(ac_expr *expr)
{
    if (expr)
    {
        switch (expr->type)
        {
            case EXPR_CALL:
                ac_expr_free_expression(expr->u.call.callee);
                if (expr->u.call.arguments)
                {
                    for (int i = 0; i < expr->u.call.arg_count; i++)
                        ac_expr_free_expression(expr->u.call.arguments[i]);
                    ac_free(expr->u.call.arguments);
                }
                break;
            case EXPR_FIELD:
                ac_expr_free_expression(expr->u.field.object);
                break;
            case EXPR_INDEX:
                ac_expr_free_expression(expr->u.index.object);
                ac_expr_free_expression(expr->u.index.index);
                break;
            case EXPR_BINARY:
                ac_expr_free_expression(expr->u.binary.left);
                ac_expr_free_expression(expr->u.binary.right);
                break;
            case EXPR_UNARY:
                ac_expr_free_expression(expr->u.unary.right);
                break;
            case EXPR_GROUPING:
                ac_expr_free_expression(expr->u.grouping.expression);
                break;
            default:;
        }
        ac_free(expr);
    }
}

void ac_expr_free_rule(ac_statement *rule)
{
    ac_expr_free_expression(rule->u.rule.condition);
    ac_free(rule);
}

void ac_expr_free_sequence(ac_statement *seq)
{
    for (int i = 0; i < seq->u.sequence.rule_count; i++)
    {
        if (seq->u.sequence.rules[i])
            ac_expr_free_rule(seq->u.sequence.rules[i]);
    }
    if (seq->u.sequence.rule_count)
        ac_free(seq->u.sequence.rules);
    ac_free(seq);
}

void ac_expr_free_import(ac_statement *import)
{
    ac_free(import);
}

void ac_expr_free_ast(ac_ast *program)
{
    if (program)
    {
        if (program->statements)
        {
            for (int i = 0; i < program->stmt_count; i++)
            {
                if (program->statements[i])
                {
                    switch (program->statements[i]->type)
                    {
                        case STMT_RULE: ac_expr_free_rule(program->statements[i]); break;
                        case STMT_SEQUENCE: ac_expr_free_sequence(program->statements[i]); break;
                        case STMT_IMPORT: ac_expr_free_import(program->statements[i]); break;
                    }
                }
            }
            ac_free(program->statements);
        }
        ac_free(program);
    }
}
