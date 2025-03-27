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
#include <string.h>
#include <alca/arena.h>
#include <alca/errors.h>
#include <alca/bytecode.h>
#include <alca/context.h>
#include <alca/types.h>
#include <alca/utils.h>

ac_error bytecode_emit_expr(ac_builder *builder, ac_expr *expr);

#define patch(offset, op, addr) \
    char inst[5] = {0}; \
    inst[0] = op; \
    *(uint32_t *)(inst + 1) = addr; \
    ac_arena_replace_bytes(builder->code, offset, inst, 5); \

ac_error bytecode_emit_unary(ac_builder *builder, ac_expr *expr)
{
    ac_error err = bytecode_emit_expr(builder, expr->u.unary.right);
    if (err != ERROR_SUCCESS)
        return err;
    switch (expr->u.unary.op->type)
    {
        case TOKEN_BANG:
        case TOKEN_NOT: ac_arena_add_code(builder->code, OP_NOTL); break;
        case TOKEN_BITNOT: ac_arena_add_code(builder->code, OP_NOT); break;
        case TOKEN_MINUS: ac_arena_add_code(builder->code, OP_NEG); break;
        case TOKEN_HASH: ac_arena_add_code(builder->code, OP_STRLEN); break;
        default: ;
    }
    return ERROR_SUCCESS;
}

ac_error bytecode_emit_binary(ac_builder *builder, ac_expr *expr)
{
    ac_error err;
    uint32_t offset = 0;

    // emit LHS first
    err = bytecode_emit_expr(builder, expr->u.binary.left);
    if (err != ERROR_SUCCESS)
        return err;
    ac_token_type op_type = expr->u.binary.op->type;

    // check that LHS is TRUE.
    // JMP to RHS code if it is. push FALSE for RHS result if it isn't, then jump over RHS
    if (op_type == TOKEN_AND)
    {
        uint32_t jmpaddr = ac_arena_size(builder->code) + (5 * 3); // jump over self, "PUSH FALSE" and "JUMP" insts
        ac_arena_add_code_with_arg(builder->code, OP_JTRUE, jmpaddr); // jump if true
        ac_arena_add_code_with_arg(builder->code, OP_PUSHBOOL, 0);
        offset = ac_arena_add_code_with_arg(builder->code, OP_JMP, 0); // arg is placeholder
    }
    // emit RHS
    err = bytecode_emit_expr(builder, expr->u.binary.right);
    if (err != ERROR_SUCCESS)
        return err;

    // patch JMP over RHS to end of RHS expression
    if (op_type == TOKEN_AND)
    {
        uint32_t jmpaddr = ac_arena_size(builder->code);
        char inst[5] = {0};
        inst[0] = OP_JMP;
        *(uint32_t *)(inst + 1) = jmpaddr;
        ac_arena_replace_bytes(builder->code, offset, inst, 5); // patch jump to after RHS code is compiled
    }
    switch (expr->u.binary.op->type)
    {
        case TOKEN_AND: ac_arena_add_code(builder->code, OP_ANDL); break;
        case TOKEN_OR: ac_arena_add_code(builder->code, OP_ORL); break;

        case TOKEN_PLUS: ac_arena_add_code(builder->code, OP_ADD); break;
        case TOKEN_MINUS: ac_arena_add_code(builder->code, OP_SUB); break;
        case TOKEN_MULT: ac_arena_add_code(builder->code, OP_MUL); break;
        case TOKEN_DIV: ac_arena_add_code(builder->code, OP_DIV); break;
        case TOKEN_MOD: ac_arena_add_code(builder->code, OP_MOD); break;

        case TOKEN_SHL: ac_arena_add_code(builder->code, OP_SHL); break;
        case TOKEN_SHR: ac_arena_add_code(builder->code, OP_SHR); break;
        case TOKEN_BITAND: ac_arena_add_code(builder->code, OP_AND); break;
        case TOKEN_BITXOR: ac_arena_add_code(builder->code, OP_XOR); break;
        case TOKEN_PIPE: ac_arena_add_code(builder->code, OP_OR); break;

        case TOKEN_CONTAINS: ac_arena_add_code(builder->code, OP_CONTAINS); break;
        case TOKEN_ICONTAINS: ac_arena_add_code(builder->code, OP_ICONTAINS); break;
        case TOKEN_STARTSWITH: ac_arena_add_code(builder->code, OP_STARTSWITH); break;
        case TOKEN_ISTARTSWITH: ac_arena_add_code(builder->code, OP_ISTARTSWITH); break;
        case TOKEN_ENDSWITH: ac_arena_add_code(builder->code, OP_ENDSWITH); break;
        case TOKEN_IENDSWITH:ac_arena_add_code(builder->code, OP_IENDSWITH); break;
        case TOKEN_IEQUALS: ac_arena_add_code(builder->code, OP_IEQUALS); break;
        case TOKEN_MATCHES: ac_arena_add_code(builder->code, OP_MATCHES); break;

        case TOKEN_GREATER: ac_arena_add_code(builder->code, OP_GT); break;
        case TOKEN_LESS: ac_arena_add_code(builder->code, OP_LT); break;
        case TOKEN_GREATER_EQUAL: ac_arena_add_code(builder->code, OP_GTE); break;
        case TOKEN_LESS_EQUAL: ac_arena_add_code(builder->code, OP_LTE); break;
        case TOKEN_EQUAL_EQUAL:
            if (expr->u.binary.operand_type == TOKEN_STRING)
                ac_arena_add_code(builder->code, OP_STREQ);
            else if (expr->u.binary.operand_type == TOKEN_TRUE || expr->u.binary.operand_type == TOKEN_FALSE)
                ac_arena_add_code(builder->code, OP_BOOLEQ);
            else
                ac_arena_add_code(builder->code, OP_INTEQ);
            break;
        case TOKEN_BANG_EQUAL:
            if (expr->u.binary.operand_type == TOKEN_STRING)
                ac_arena_add_code(builder->code, OP_STRNE);
            else if (expr->u.binary.operand_type == TOKEN_TRUE || expr->u.binary.operand_type == TOKEN_FALSE)
                ac_arena_add_code(builder->code, OP_BOOLNE);
            else
                ac_arena_add_code(builder->code, OP_INTNE);
            break;
        default: ;
    }
    return ERROR_SUCCESS;
}

void bytecode_emit_literal(ac_builder *builder, ac_expr *expr)
{
    if (expr->u.literal.value->type == TOKEN_IDENTIFIER)
    {
        uint32_t offset = ac_arena_add_string(builder->data,
            expr->u.literal.value->value,
            strlen(expr->u.literal.value->value));
        if (builder->module_name)
        {
            // if it's the module, push module onto stack
            if (strcmp(expr->u.literal.value->value, builder->module_name) == 0)
                ac_arena_add_code_with_arg(builder->code, OP_PUSHMODULE, offset);
            // if it's an iterator variable, don't do anything, it's pushed on the stack at the beginning
            // of each loop
            else if (strcmp(expr->u.literal.value->value, builder->iter) == 0)
            {
                ac_arena_add_code_with_arg(builder->code, OP_LOAD, 0); // iterator register
            }
            else
                // otherwise, it's a rule. run it (pushes bool onto stack)
                ac_arena_add_code_with_arg(builder->code, OP_RULE, offset);
        }
    } else if (expr->u.literal.value->type == TOKEN_NUMBER)
    {
        ac_arena_add_code_with_arg(builder->code, OP_PUSHINT, *(uint32_t *)expr->u.literal.value->value);
    } else if (expr->u.literal.value->type == TOKEN_STRING)
    {
        uint32_t offset = ac_arena_add_string(builder->data,
            expr->u.literal.value->value,
            strlen(expr->u.literal.value->value));
        ac_arena_add_code_with_arg(builder->code, OP_PUSHSTRING, offset);
    } else if (expr->u.literal.value->type == TOKEN_REGEX)
    {
        uint32_t offset = ac_arena_add_string(builder->data,
            expr->u.literal.value->value,
            strlen(expr->u.literal.value->value));
        ac_arena_add_code_with_arg(builder->code, OP_PUSHSTRING, offset); // push match string
        ac_arena_add_code_with_arg(builder->code, OP_PUSHINT, expr->u.literal.value->flags); // push modifier flags
    } else if (expr->u.literal.value->type == TOKEN_TRUE)
    {
        ac_arena_add_code_with_arg(builder->code, OP_PUSHBOOL, TRUE);
    } else if (expr->u.literal.value->type == TOKEN_FALSE)
    {
        ac_arena_add_code_with_arg(builder->code, OP_PUSHBOOL, FALSE);
    }
}

ac_error bytecode_emit_call(ac_builder *builder, ac_expr *expr)
{
    ac_error error = ERROR_SUCCESS;
    // push args right to left
    for (int i = (int)expr->u.call.arg_count - 1; i >= 0; i--)
    {
        if ((error = bytecode_emit_expr(builder, expr)) != ERROR_SUCCESS)
            return error;
    }
    ac_arena_add_code_with_arg(builder->code, OP_PUSHINT, expr->u.call.arg_count);
    // pop a1; pop a2; ...; pop aN; pop call_object; push call_object(a1, a2, ... aN)
    ac_arena_add_code(builder->code, OP_CALL);
    return error;
}

// field operation expects an object to be on the stack.
// the result of the operation is pushing the sub-object with the name matching the operand onto the stack.
void bytecode_emit_field(ac_builder *builder, ac_expr *expr)
{
    bytecode_emit_expr(builder, expr->u.field.object);
    uint32_t offset = ac_arena_add_string(builder->data,
        expr->u.field.field_name->value,
        strlen(expr->u.field.field_name->value));
    ac_arena_add_code_with_arg(builder->code, OP_FIELD, offset);
    ac_field_type type = expr->u.field.identifier_type;
    // push the stack object if field is being used as a literal
    if (!(type & AC_FIELD_TYPE_ARRAY || type & AC_FIELD_TYPE_STRUCT || type & AC_FIELD_TYPE_FUNCTION))
    {
        ac_arena_add_code(builder->code, OP_OBJECT);
    }
}

ac_error bytecode_emit_index(ac_builder *builder, ac_expr *expr)
{
    ac_error error = bytecode_emit_expr(builder, expr->u.index.object);
    if (error != ERROR_SUCCESS)
        return error;
    error = bytecode_emit_expr(builder, expr->u.index.index);
    if (error != ERROR_SUCCESS)
        return error;
    ac_arena_add_code(builder->code, OP_INDEX); // pop idx; pop array_object; push array_object[idx]
    return error;
}

void bytecode_emit_range(ac_builder *builder, ac_expr *expr)
{
    // for (i = range_start; i < range_end; i++)
    uint32_t offset = 0;
    builder->iter = expr->u.range.ivar->value;
    bytecode_emit_expr(builder, expr->u.range.end); // push end onto stack
    bytecode_emit_expr(builder, expr->u.range.start); // push start onto stack

    ac_arena_add_code_with_arg(builder->code, OP_STORE, 0); // save start value into accumulator 0
    ac_arena_add_code_with_arg(builder->code, OP_STORE, 1); // save end value into accumulator 1

    // i < range_end
    uint32_t iter_start = ac_arena_add_code_with_arg(builder->code, OP_LOAD, 0);
    ac_arena_add_code_with_arg(builder->code, OP_LOAD, 1);
    ac_arena_add_code(builder->code, OP_LT);
    // placeholder, jump to end of processing with FALSE on stack
    offset = ac_arena_add_code_with_arg(builder->code, OP_JFALSE, 0);
    ac_arena_add_code(builder->code, OP_POP); // discard result of OP_LT

    bytecode_emit_expr(builder, expr->u.range.condition); // pushes BOOL
    // saves expr bool to accum 2
    ac_arena_add_code_with_arg(builder->code, OP_STORE, 2);
    ac_arena_add_code_with_arg(builder->code, OP_LOAD, 2);
    if (expr->u.range.all)
    {
        // EXIT loop if expr is false, with FALSE on the stack
        ac_arena_add_code_with_arg(builder->code, OP_JFALSE, ac_arena_size(builder->code) + 27);
    } else if (expr->u.range.any)
    {
        // EXIT loop if expr is true, with TRUE on the stack
        ac_arena_add_code_with_arg(builder->code, OP_JTRUE, ac_arena_size(builder->code) + 27);
    } else
    {
        // update accum if condition is true
        ac_arena_add_code_with_arg(builder->code, OP_JTRUE, ac_arena_size(builder->code) + 10);
        // otherwise, loop
        uint32_t offset2 = ac_arena_add_code_with_arg(builder->code, OP_JMP, 0);

        // add 1 to accumulator 3
        ac_arena_add_code(builder->code, OP_POP); // discard range expression result
        ac_arena_add_code_with_arg(builder->code, OP_LOAD, 3); // load match count from accumulator 3
        ac_arena_add_code_with_arg(builder->code, OP_PUSHINT, 1);
        ac_arena_add_code(builder->code, OP_ADD);
        ac_arena_add_code_with_arg(builder->code, OP_STORE, 3);
        // check accumulator 3 is greater than or equal to the required condition count
        ac_arena_add_code_with_arg(builder->code, OP_LOAD, 3);
        ac_arena_add_code_with_arg(builder->code, OP_PUSHINT, expr->u.range.fixed);
        ac_arena_add_code(builder->code, OP_GTE);
        // EXIT loop if accum is greater or equal, with TRUE on the stack
        ac_arena_add_code_with_arg(builder->code, OP_JTRUE, ac_arena_size(builder->code) + 27);

        patch(offset2, OP_JMP, ac_arena_size(builder->code));
    }
    ac_arena_add_code(builder->code, OP_POP); // discard expression range result
    // i++
    ac_arena_add_code_with_arg(builder->code, OP_LOAD, 0);
    ac_arena_add_code_with_arg(builder->code, OP_PUSHINT, 1);
    ac_arena_add_code(builder->code, OP_ADD);
    ac_arena_add_code_with_arg(builder->code, OP_STORE, 0); // save i
    ac_arena_add_code_with_arg(builder->code, OP_JMP, iter_start);

    // patch EXIT jump over range expression
    uint32_t jmpaddr = ac_arena_size(builder->code);
    char inst[5] = {0};
    inst[0] = OP_JFALSE;
    *(uint32_t *)(inst + 1) = jmpaddr;
    ac_arena_replace_bytes(builder->code, offset, inst, 5);

    ac_arena_add_code(builder->code, OP_POP);
    ac_arena_add_code_with_arg(builder->code, OP_LOAD, 2); // load expr result
    builder->iter = NULL;
}

ac_error bytecode_emit_expr(ac_builder *builder, ac_expr *expr)
{
    ac_error error = ERROR_SUCCESS;
    switch (expr->type)
    {
        case EXPR_BINARY: error = bytecode_emit_binary(builder, expr); break;
        case EXPR_GROUPING: error = bytecode_emit_expr(builder, expr->u.grouping.expression); break;
        case EXPR_UNARY: error = bytecode_emit_unary(builder, expr); break;
        case EXPR_LITERAL: bytecode_emit_literal(builder, expr); break;
        case EXPR_CALL: error = bytecode_emit_call(builder, expr); break;
        case EXPR_FIELD: bytecode_emit_field(builder, expr); break;
        case EXPR_INDEX: bytecode_emit_index(builder, expr); break;
        case EXPR_RANGE: bytecode_emit_range(builder, expr); break;
    }
    return error;
}

ac_error ac_bytecode_emit_rule(ac_builder *builder, ac_statement *rule)
{
    ac_module_load_callback callback;
    if (rule->u.rule.event)
    {
        if (!ac_context_get_module(builder->ctx, rule->u.rule.event->value, &callback))
            return ERROR_MODULE;
        builder->module_name = rule->u.rule.event->value;
    }
    if (rule->u.rule.external)
        return ERROR_SUCCESS; // external rules have no code to emit
    ac_error err = bytecode_emit_expr(builder, rule->u.rule.condition);
    if (err != ERROR_SUCCESS)
        return err;
    ac_arena_add_code(builder->code, OP_HLT); // indicates end of rule. vm exits loop when it sees halt
    return ERROR_SUCCESS;
}
