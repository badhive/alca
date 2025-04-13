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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <alca/errors.h>
#include <alca/utils.h>
#include "hashmap.h"

#include <alca/checker.h>

struct ac_checker
{
    ac_context *ctx;
    ac_context *rule_ctx;
    ac_statement *current_rule;
    int nerrors;
    char **error_msgs;
    int *error_lines;
    ac_error *error_codes;
    int npriv_vars;
    char **priv_vars;
    ac_ast *ast;
    int depth;
    int iter;
    struct hashmap *env;
};

struct chain
{
    char *name;
    int line;
    ac_expr_type type;
    ac_expr *expr;
    ac_expr *extra;
    struct chain *next;
};

ac_token_type resolve_type(ac_checker *checker, ac_expr *expr);

#define report_error(ch, token, code, ret, ...) \
    {checker_errorf(ch, token->line, code, __VA_ARGS__); return ret;}

void checker_add_private_rule(ac_checker *checker, char *rule_name)
{
    if (!checker->priv_vars)
        checker->priv_vars = ac_alloc(sizeof(char *));
    else
        checker->priv_vars = ac_realloc(checker->priv_vars, sizeof(char *) * (checker->npriv_vars + 1));
    checker->priv_vars[checker->npriv_vars++] = rule_name;
}

// clears private variables from hashmap as they are only valid for current file's scope
void checker_free_private_vars(ac_checker *checker)
{
    for (int i = 0; i < checker->npriv_vars; i++)
        hashmap_delete(checker->env, &(ac_context_env_item){.name = checker->priv_vars[i]});
}

void checker_add_error(ac_checker *checker, char *msg, int line, ac_error code)
{
    // actual error messages
    if (!checker->error_msgs)
        checker->error_msgs = ac_alloc(sizeof(char *));
    else
        checker->error_msgs = ac_realloc(checker->error_msgs, sizeof(char *) * (checker->nerrors + 1));
    checker->error_msgs[checker->nerrors] = msg;

    // error line numbers
    if (!checker->error_lines)
        checker->error_lines = ac_alloc(sizeof(int));
    else
        checker->error_lines = ac_realloc(checker->error_lines, sizeof(int) * (checker->nerrors + 1));
    checker->error_lines[checker->nerrors] = line;

    // error codes
    if (!checker->error_codes)
        checker->error_codes = ac_alloc(sizeof(ac_error));
    else
        checker->error_codes = ac_realloc(checker->error_codes, sizeof(int) * (checker->nerrors + 1));
    checker->error_codes[checker->nerrors] = code;
    checker->nerrors++;
}

void checker_errorf(ac_checker *checker, int line, ac_error code, const char *message, ...)
{
    va_list args, args_copy;
    va_start(args, message);
    va_copy(args_copy, args);
    int msg_len = vsnprintf(NULL, 0, message, args_copy);
    char *msg = ac_alloc(msg_len + 1);
    vsnprintf(msg, msg_len + 1, message, args);
    va_end(args);
    va_end(args_copy);
    int fullmsg_len = snprintf(NULL, 0, "%s: error (line %d): %s", checker->ast->path, line, msg);
    char *fullmsg = ac_alloc(fullmsg_len + 1);
    snprintf(fullmsg, fullmsg_len + 1, "%s: error (line %d): %s", checker->ast->path, line, msg);
    ac_free(msg);
    checker_add_error(checker, fullmsg, line, code);
}

int ac_checker_iter_errors(ac_checker *checker, int *line, ac_error *code, char **errmsg)
{
    if (checker->iter >= checker->nerrors)
        return FALSE;
    *errmsg = checker->error_msgs[checker->iter];
    *code = checker->error_codes[checker->iter];
    *line = checker->error_lines[checker->iter];
    checker->iter++;
    return TRUE;
}

int checker_valid_call(ac_checker *checker, ac_expr *call, ac_context_object *function)
{
    if (!call) return FALSE;
    char *arg_types = NULL;
    for (int i = 0; i < call->u.call.arg_count; i++)
    {
        ac_token_type t = resolve_type(checker, call->u.call.arguments[i]);
        if (t == TOKEN_EOF)
            return FALSE;
        if (t == TOKEN_STRING)
            arg_types = ac_str_extend(arg_types, 's');
        else if (t == TOKEN_NUMBER)
            arg_types = ac_str_extend(arg_types, 'i');
        else if (t == TOKEN_FALSE || t == TOKEN_TRUE)
            arg_types = ac_str_extend(arg_types, 'b');
        else
            report_error(checker, call->u.call.paren, AC_ERROR_UNEXPECTED_TYPE, FALSE,
                     "argument must be string, integer or boolean");
    }
    unsigned int count;
    const char *types;
    ac_error error = ac_context_validate_function_call(function, arg_types, &count, &types);
    if (arg_types) ac_free(arg_types);
    if (error != AC_ERROR_SUCCESS)
    {
        char *msg = ac_alloc(32);
        if (error == AC_ERROR_BAD_CALL)
            sprintf(msg, "expected %d arguments", count);
        else if (error == AC_ERROR_UNEXPECTED_TYPE)
        {
            report_error(checker, call->u.call.paren, AC_ERROR_BAD_CALL, FALSE,
                         "expected argument types '%s', got '%s'", types, arg_types);
        } else
            report_error(checker, call->u.call.paren, AC_ERROR_BAD_CALL, FALSE,
                     "not a function");
    }
    return TRUE;
}

ac_token_type resolve_identifier_type_from_context(ac_checker *checker, struct chain *node, ac_context_object *object)
{
    if (!node)
        return TOKEN_EOF; // in case somehow resolve_identifier_type returns null node. TODO investigate later
    char *name;
    ac_field_type type;
    ac_context_object *subobject = ac_context_object_get(object, node->name);
    if (!subobject)
    {
        ac_context_object_info(object, &name, &type);
        report_error(checker, node, AC_ERROR_UNKNOWN_FIELD, TOKEN_EOF, "unknown field '%s' for %s", node->name, name);
    }
    ac_context_object_info(subobject, &name, &type);
    // if object is used on its own without accessing fields / indices (no next pointer)
    if (type & AC_FIELD_TYPE_STRUCT && !node->next)
    {
        if (type & AC_FIELD_TYPE_ARRAY)
        {
            report_error(checker, node, AC_ERROR_BAD_LITERAL, TOKEN_EOF, "item in '%s' cannot be used as a literal",
                         node->name);
        }
        if (type & AC_FIELD_TYPE_FUNCTION)
        {
            report_error(checker, node, AC_ERROR_BAD_LITERAL, TOKEN_EOF, "%s's return value cannot be used as a literal",
                         node->name);
        }
        report_error(checker, node, AC_ERROR_BAD_LITERAL, TOKEN_EOF, "'%s' cannot be used as a literal", node->name);
    }
    // if an item type is EXPR_FIELD, then it is being used as a literal (no call, index, property etc. after its name).
    // this is not allowed for arrays, objects or functions.
    if (node->type == EXPR_FIELD)
    {
        if (type & AC_FIELD_TYPE_ARRAY || type & AC_FIELD_TYPE_STRUCT || type & AC_FIELD_TYPE_FUNCTION)
        {
            report_error(checker, node, AC_ERROR_BAD_LITERAL, TOKEN_EOF, "'%s' cannot be used as a literal", node->name);
        }
        node->expr->u.field.identifier_type = type;
    } else if (node->type == EXPR_INDEX) // if the index is performed on non-array
    {
        if (!(type & AC_FIELD_TYPE_ARRAY))
        {
            report_error(checker, node, AC_ERROR_NOT_SUBSCRIPTABLE, TOKEN_EOF, "'%s' is not subscriptable", node->name);
        }
        node->expr->u.index.item_type = type;
        node->extra->u.field.identifier_type = type;
    } else if (node->type == EXPR_CALL)
    {
        // if the call is performed on non-function
        if (!(type & AC_FIELD_TYPE_FUNCTION))
        {
            report_error(checker, node, AC_ERROR_BAD_CALL, TOKEN_EOF, "'%s' is not callable", node->name);
        }
        if (!checker_valid_call(checker, node->expr, subobject))
        {
            report_error(checker, node, AC_ERROR_UNEXPECTED_TYPE, TOKEN_EOF, "invalid arguments for function '%s'",
                         node->name);
        }
        node->expr->u.call.return_type = type;
        node->extra->u.field.identifier_type = type;
    }
    if (!(type & AC_FIELD_TYPE_STRUCT))
    {
        if (type & AC_FIELD_TYPE_STRING)
            return TOKEN_STRING;
        if (type & AC_FIELD_TYPE_INTEGER)
            return TOKEN_NUMBER;
        if (type & AC_FIELD_TYPE_BOOLEAN)
            return TOKEN_TRUE;
        return TOKEN_EOF;
    }
    return resolve_identifier_type_from_context(checker, node->next, subobject);
}

ac_token_type resolve_identifier_type(ac_checker *checker, ac_expr *expr)
{
    ac_expr *expr_parent = expr;
    ac_expr *current_expr;
    struct chain *head = NULL;
    ac_token_type ret;

    // rules are the only top-level identifiers that can be used on their own as a literal (bool)
    if (expr->type == EXPR_LITERAL)
    {
        const ac_context_env_item *item = hashmap_get(checker->env,
                                                      &(ac_context_env_item){.name = expr->u.literal.value->value});
        if (item)
        {
            if (item->type == STMT_RULE)
            {
                // catch recursive rule calls
                if (checker->current_rule && strcmp(item->name, checker->current_rule->u.rule.name->value) == 0)
                    report_error(checker, expr_parent->u.literal.value, AC_ERROR_RECURSION,
                             TOKEN_EOF,
                             "rule cannot reference itself");
                return TOKEN_TRUE; // rules return boolean
            }
            report_error(checker, expr->u.literal.value, TOKEN_EOF, AC_ERROR_BAD_LITERAL, "cannot use name as literal");
        }
    }
    while (expr_parent->type != EXPR_LITERAL) // a literal expression means we reached the module / global name
    {
        char *name = NULL;
        int line = 0;
        ac_expr_type curr_type = expr_parent->type;
        ac_expr *extra = NULL;
        current_expr = expr_parent;
        switch (curr_type)
        {
            case EXPR_FIELD:
            {
                ac_expr *object = expr_parent->u.field.object;
                while (object->type == EXPR_GROUPING)
                {
                    object = object->u.grouping.expression;
                }
                if (object->type != EXPR_LITERAL &&
                    object->type != EXPR_FIELD &&
                    object->type != EXPR_CALL &&
                    object->type != EXPR_INDEX)
                    report_error(checker, expr_parent->u.field.field_name,
                             AC_ERROR_FIELD_ACCESS,
                             TOKEN_EOF, "cannot get property of object");
                name = expr_parent->u.field.field_name->value;
                line = expr_parent->u.field.field_name->line;
                expr_parent = expr_parent->u.field.object;
                break;
            }
            case EXPR_CALL:
            {
                ac_expr *callee = expr_parent->u.call.callee;
                line = expr_parent->u.call.paren->line;
                // ensure that parent object is an identifier (i.e. the module itself)
                if (callee->type == EXPR_FIELD)
                {
                    if (callee->u.field.object->type != EXPR_LITERAL || callee->u.field.object->u.literal.value->type !=
                        TOKEN_IDENTIFIER)
                        report_error(checker, expr->u.call.paren,
                                 AC_ERROR_BAD_CALL, TOKEN_EOF,
                                 "methods and anonymous functions not supported");
                    name = callee->u.field.field_name->value;
                    // since we skip the callee name, we need to have it saved in extra
                    extra = callee;
                    expr_parent = callee->u.field.object;
                } else if (callee->type == EXPR_LITERAL) // top-level functions not supported
                {
                    report_error(checker, callee->u.call.paren, AC_ERROR_BAD_CALL, TOKEN_EOF, "literal is not callable");
                } else // ensure that callees are either fields or literals only
                    report_error(checker, expr->u.call.paren,
                             AC_ERROR_BAD_CALL, TOKEN_EOF,
                             "methods and anonymous functions not supported");
                break;
            }
            case EXPR_INDEX:
            {
                ac_expr *array = expr_parent->u.index.object;
                if (array->type != EXPR_FIELD)
                    report_error(checker, expr_parent->u.index.bracket,
                             AC_ERROR_NOT_SUBSCRIPTABLE, TOKEN_EOF, "only array objects can be indexed");
                line = expr_parent->u.index.bracket->line;
                name = array->u.field.field_name->value;
                // since we skip the array name, we need to save it in extra
                extra = array;
                expr_parent = array->u.field.object;
                break;
            }
            default: ;
        }
        struct chain *item = ac_alloc(sizeof(struct chain));
        item->name = name;
        item->line = line;
        item->type = curr_type;
        item->expr = current_expr;
        item->extra = extra;
        item->next = head;
        head = item;
        extra = NULL;
    }
    const char *name = expr_parent->u.literal.value->value;
    ac_context_object *object;

    if (expr_parent->u.literal.value->type != TOKEN_IDENTIFIER)
        report_error(checker, expr_parent->u.literal.value, AC_ERROR_FIELD_ACCESS, TOKEN_EOF,
                 "cannot get property of literal");


    // catch unknown identifiers. need to check rule_ctx in case an identifier is top-level and not a module name
    // if event hasn't been brought into scope, then rule_ctx won't exist
    if (!checker->rule_ctx || (object = ac_context_get(checker->rule_ctx, name)) == NULL)
    {
        if (ac_context_get(checker->ctx, name))
        {
            // if module exists but not in rule scope
            report_error(checker, expr_parent->u.literal.value, AC_ERROR_UNKNOWN_IDENTIFIER, TOKEN_EOF,
                         "'%s' event not in rule scope", name);
        }
        // if no module with that name exists
        report_error(checker, expr_parent->u.literal.value, AC_ERROR_UNKNOWN_IDENTIFIER, TOKEN_EOF,
                     "undefined symbol '%s'", name);
    }
    if (!object)
    {
        // if no module with that name exists
        report_error(checker, expr_parent->u.literal.value, AC_ERROR_UNKNOWN_IDENTIFIER, TOKEN_EOF,
                     "unknown identifier '%s'", name);
    }
    ret = resolve_identifier_type_from_context(checker, head, object);
    for (struct chain *field_list = head; field_list;)
    {
        struct chain *tmp = field_list->next;
        ac_free(field_list);
        field_list = tmp;
    }
    return ret;
}

int valid_operation(ac_token_type type1, ac_token_type type2, ac_token_type op, ac_token_type *result)
{
    int valid = FALSE;
    *result = type1;
    switch (op)
    {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_DIV:
        case TOKEN_MULT:
        case TOKEN_SHR:
        case TOKEN_SHL:
        case TOKEN_BITAND:
        case TOKEN_BITXOR:
        case TOKEN_BITNOT:
        case TOKEN_PIPE:
        case TOKEN_MOD:
            valid = type1 == TOKEN_NUMBER;
            break;
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL:
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
            valid = type1 == TOKEN_NUMBER;
            *result = TOKEN_TRUE; // boolean for comparison
            break;
        case TOKEN_AND:
        case TOKEN_OR:
        case TOKEN_NOT:
        case TOKEN_BANG:
            valid = type1 == TOKEN_TRUE || type1 == TOKEN_FALSE;
            break;
        // these are the only two comparisons that apply to strings, ints and bools. thus type information
        // must be saved on binary expressions involving one of these operations.
        case TOKEN_BANG_EQUAL:
        case TOKEN_EQUAL_EQUAL:
            valid = type1 == TOKEN_TRUE ||
                    type1 == TOKEN_FALSE ||
                    type1 == TOKEN_NUMBER ||
                    type1 == TOKEN_STRING;
            *result = TOKEN_TRUE;
            break;
        // string ops
        case TOKEN_CONTAINS:
        case TOKEN_ICONTAINS:
        case TOKEN_STARTSWITH:
        case TOKEN_ISTARTSWITH:
        case TOKEN_ENDSWITH:
        case TOKEN_IENDSWITH:
        case TOKEN_IEQUALS:
            valid = type1 == TOKEN_STRING;
            *result = TOKEN_TRUE;
            break;
        case TOKEN_MATCHES:
            valid = type1 == TOKEN_STRING && type2 == TOKEN_REGEX;
            *result = TOKEN_TRUE;
            break;
        case TOKEN_HASH: // strlen
            valid = type1 == TOKEN_STRING;
            *result = TOKEN_NUMBER;
            break;
        default:
            valid = FALSE;
    }
    return valid;
}

ac_token_type resolve_type(ac_checker *checker, ac_expr *expr)
{
    switch (expr->type)
    {
        case EXPR_FIELD: // fall through
        case EXPR_CALL: return resolve_identifier_type(checker, expr);
        case EXPR_GROUPING: return resolve_type(checker, expr->u.grouping.expression);
        case EXPR_INDEX:
        {
            ac_token_type type = resolve_type(checker, expr->u.index.index);
            if (type != TOKEN_NUMBER)
                report_error(checker, expr->u.index.bracket, AC_ERROR_UNEXPECTED_TYPE, TOKEN_EOF,
                         "index must be an integer");
            return resolve_identifier_type(checker, expr);
        }
        case EXPR_LITERAL:
        {
            if (expr->u.literal.value->type != TOKEN_IDENTIFIER)
                return expr->u.literal.value->type;
            const ac_context_env_item *item = hashmap_get(checker->env,
                &(ac_context_env_item){.name = expr->u.literal.value->value});
            if (item)
            {
                // rules return true
                if (item->tok_type == 0 && item->type == STMT_RULE)
                {
                    ac_token *evt = checker->current_rule->u.rule.event;
                    // ext stores the event name for the rule
                    if (item->ext == NULL && evt != NULL || item->ext != NULL && evt == NULL)
                    {
                        report_error(checker, expr->u.literal.value, AC_ERROR_BAD_CALL, TOKEN_EOF,
                            "a referenced rule's event type must match the callee's");
                    }
                    if (item->ext && strcmp(item->ext, checker->current_rule->u.rule.event->value) != 0)
                        report_error(checker, expr->u.literal.value, AC_ERROR_BAD_CALL, TOKEN_EOF,
                            "a referenced rule's event type must match the callee's");
                    return TOKEN_TRUE;
                }
                if (item->tok_type == TOKEN_NUMBER ||
                    item->tok_type == TOKEN_STRING ||
                    item->tok_type == TOKEN_TRUE ||
                    item->tok_type == TOKEN_FALSE) // covers any local vars we may want to set
                    return item->tok_type;
            }
            return resolve_identifier_type(checker, expr);
        }
        case EXPR_BINARY:
        {
            ac_token_type t1 = resolve_type(checker, expr->u.binary.left);
            ac_token_type t2 = resolve_type(checker, expr->u.binary.right);
            if (t1 == TOKEN_EOF || t2 == TOKEN_EOF)
                return TOKEN_EOF;
            ac_token_type res = TOKEN_EOF;
            if (t1 != t2)
            {
                // true-false and string-regex are valid
                if (!(t1 == TOKEN_TRUE && t2 == TOKEN_FALSE || t2 == TOKEN_TRUE && t1 == TOKEN_FALSE) &&
                    !(t1 == TOKEN_STRING && t2 == TOKEN_REGEX))
                report_error(checker, expr->u.binary.op, AC_ERROR_BAD_OPERATION, TOKEN_EOF,
                         "invalid operation (type mismatch)");
            }
            if (!valid_operation(t1, t2, expr->u.binary.op->type, &res))
                report_error(checker, expr->u.binary.op, AC_ERROR_BAD_OPERATION, TOKEN_EOF, "incompatible operator");
            expr->u.binary.operand_type = t1;
            return res;
        }
        case EXPR_UNARY:
        {
            ac_token_type t1 = resolve_type(checker, expr->u.unary.right);
            if (t1 == TOKEN_EOF)
                return TOKEN_EOF;
            ac_token_type op = expr->u.unary.op->type;
            ac_token_type res = TOKEN_EOF;
            if (!valid_operation(t1, TOKEN_EOF, op, &res))
                report_error(checker, expr->u.unary.op, AC_ERROR_BAD_OPERATION, TOKEN_EOF,
                         "incompatible unary operator");
            return res;
        }
        case EXPR_RANGE:
        {
            ac_token_type t1 = resolve_type(checker, expr->u.range.start);
            if (t1 == TOKEN_EOF)
                return TOKEN_EOF;
            ac_token_type t2 = resolve_type(checker, expr->u.range.end);
            if (t2 == TOKEN_EOF)
                return TOKEN_EOF;
            if (t1 != t2 || t1 != TOKEN_NUMBER)
                report_error(checker, expr->u.range.ivar, AC_ERROR_BAD_OPERATION, TOKEN_EOF,
                    "start / end range values must be integers");
            char *name = expr->u.range.ivar->value;
            if (hashmap_get(checker->env, &(ac_context_env_item){.name = expr->u.range.ivar->value}))
            {
                report_error(checker, expr->u.range.ivar, AC_ERROR_REDEFINED, TOKEN_EOF,
                    "identifier '%s' has already been defined", name);
            }
            // only in scope of enclosed condition
            hashmap_set(checker->env, &(ac_context_env_item){.name = name, .tok_type = TOKEN_NUMBER});
            ac_token_type t3 = resolve_type(checker, expr->u.range.condition);
            hashmap_delete(checker->env, &(ac_context_env_item){.name = name});
            if (t3 == TOKEN_EOF)
                return TOKEN_EOF;
            if (t3 != TOKEN_TRUE && t3 != TOKEN_FALSE)
                report_error(checker, expr->u.range.ivar, AC_ERROR_UNEXPECTED_TYPE, TOKEN_EOF,
                    "expected boolean condition in range expression")
            return TOKEN_TRUE;
        }
    }
    return TOKEN_EOF;
}

int check_type(ac_checker *checker, ac_expr *expr, int line)
{
    ac_token_type type = resolve_type(checker, expr);
    if (type == TOKEN_EOF)
        return FALSE;
    if (type != TOKEN_TRUE && type != TOKEN_FALSE)
    {
        checker_errorf(checker, line, AC_ERROR_UNEXPECTED_TYPE, "rule result is not boolean");
        return FALSE;
    }
    return TRUE;
}

void checker_import(ac_checker *checker, ac_token *import_name)
{
    if (hashmap_get(checker->env, &(ac_context_env_item){import_name->value}))
        return;
    ac_context_env_item item = {.name = import_name->value, .type = STMT_IMPORT};
    hashmap_set(checker->env, &item); // add import to environment
    //ac_context_create_object(checker->ctx, item.name);
}

int checker_check_rule(ac_checker *checker, ac_statement *stmt, int is_seq_rule)
{
    int ret = TRUE;
    int line = stmt->u.rule.name->line;
    checker->current_rule = stmt;
    if (!is_seq_rule)
    {
        // check if rule is already defined
        if (hashmap_get(checker->env, &(ac_context_env_item){.name = stmt->u.rule.name->value}))
        {
            checker_errorf(checker, stmt->u.rule.name->line, AC_ERROR_REDEFINED, "name '%s' already defined",
                           (char *) stmt->u.rule.name->value);
            ret = FALSE;
            goto end;
        }
        // add rule name to environment
        ac_context_env_item rule_name = {
            .name = stmt->u.rule.name->value,
            .type = STMT_RULE,
            .src = (char*)checker->ast->path,
        };
        if (stmt->u.rule.event)
            rule_name.ext = stmt->u.rule.event->value;
        hashmap_set(checker->env, &rule_name);

        if (stmt->u.rule.is_private)
            // will be removed from scope once file is done checking, so other rule files cannot use it
            checker_add_private_rule(checker, rule_name.name);
    }
    if (stmt->u.rule.event)
    {
        ac_module_table_entry module = {0};
        if (!ac_context_get_module(checker->ctx, stmt->u.rule.event->value, &module))
        {
            checker_errorf(checker, stmt->u.rule.event->line, AC_ERROR_MODULE, "module %s does not exist",
                stmt->u.rule.event->value);
            ret = FALSE;
            goto end;
        }
        // load module into context, brings into rule scope
        checker->rule_ctx = ac_context_new();
        ac_context_add_module(checker->rule_ctx, &module);
        ac_context_load_modules(checker->rule_ctx);
    }
    if (!check_type(checker, stmt->u.rule.condition, line))
        ret = FALSE;
end:
    if (checker->rule_ctx)
    {
        ac_context_free(checker->rule_ctx);
        checker->rule_ctx = NULL;
    }
    checker->current_rule = NULL;
    return ret;
}

int checker_check_sequence(ac_checker *checker, ac_statement *stmt)
{
    if (hashmap_get(checker->env, &(ac_context_env_item){.name = stmt->u.sequence.name->value}))
    {
        checker_errorf(checker, stmt->u.sequence.name->line, AC_ERROR_REDEFINED, "name '%s' already defined",
                       (char *) stmt->u.sequence.name->value);
        return FALSE;
    }
    ac_context_env_item item = {.name = stmt->u.sequence.name->value, .type = STMT_SEQUENCE};
    item.name = stmt->u.sequence.name->value;
    hashmap_set(checker->env, &item); // add sequence to environment
    for (int i = 0; i < stmt->u.sequence.rule_count; i++)
    {
        int line = stmt->u.sequence.rules[i]->u.rule.name->line; // valid for external rule, name token is an lbrace
        if (!stmt->u.sequence.rules[i]->u.rule.external)
        {
            if (!checker_check_rule(checker, stmt->u.sequence.rules[i], TRUE))
                return FALSE;
        } else
        {
            ac_statement *rule = stmt->u.sequence.rules[i];
            const ac_context_env_item *rule_id = hashmap_get(
                checker->env,
                &(ac_context_env_item){.name = rule->u.rule.name->value});
            // make sure rules from other files cannot be used in a sequence
            if (!rule_id || rule_id->type != STMT_RULE || strcmp(rule_id->src, checker->ast->path) != 0)
            {
                checker_errorf(checker, line, AC_ERROR_UNKNOWN_IDENTIFIER, "undefined rule '%s' in sequence",
                               (char *) rule->u.rule.name->value);
                return FALSE;
            }
        }
    }
    return TRUE;
}

int ac_checker_check(ac_checker *checker)
{
    int failed = FALSE;
    for (int i = 0; i < checker->ast->stmt_count; i++)
    {
        ac_statement *stmt = checker->ast->statements[i];
        if (stmt->type == STMT_RULE)
        {
            if (!checker_check_rule(checker, stmt, FALSE))
                failed = TRUE;
        } else if (stmt->type == STMT_SEQUENCE)
        {
            if (!checker_check_sequence(checker, stmt))
                failed = TRUE;
        } else if (stmt->type == STMT_IMPORT)
            checker_import(checker, stmt->u.import.name);
    }
    checker_free_private_vars(checker);
    return !failed;
}

ac_checker *ac_checker_new(ac_ast *ast, ac_context *ctx)
{
    ac_checker *checker = ac_alloc(sizeof(ac_checker));
    checker->ast = ast;
    checker->ctx = ctx;
    checker->env = ac_context_get_environment(ctx);
    return checker;
}

void ac_checker_free(ac_checker *checker)
{
    int line;
    ac_error code;
    char *msg;
    while (ac_checker_iter_errors(checker, &line, &code, &msg))
        ac_free(msg);
    if (checker->error_msgs)
        ac_free(checker->error_msgs);
    if (checker->error_codes)
        ac_free(checker->error_codes);
    if (checker->error_lines)
        ac_free(checker->error_lines);
    ac_free(checker);
}
