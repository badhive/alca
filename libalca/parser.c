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
#include <alca/expr.h>
#include <alca/utils.h>

#include <alca/parser.h>

ac_expr *ac_psr_expression(ac_parser *parser);

#define MATCH( parser, ... ) ac_psr_match( parser, __VA_ARGS__, -1 )

/* Grammar (ref: https://craftinginterpreters.com/parsing-expressions.html)

program        → (sequenceStmt | ruleStmt)* EOF;

Statements
==========
sequenceStmt   → SEQUENCE IDENTIFIER "(" NUMBER ")" "[" whereStmt | IDENTIFIER ( "," whereStmt | IDENTIFIER )* "]" ;
ruleStmt       → ( PRIVATE )? RULE IDENTIFIER "{" expression "}" ;
importStmt      → "event" IDENTIFIER ;

Expressions
===========
expression     → logical ;
or             → and ( OR and )* ;
and            → not ( AND not )* ;
not            → ( NOT ) equality ;
equality       → comparison ( ( "!=" | "==" ) comparison )* ;
comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term           → factor ( ( "-" | "+" ) factor )* ;
factor         → bitand ( ( "/" | "*" | "%" ) bitand )* ;
bitand         → bitxor ( "&" bitxor )* ;
bitxor         → bitor ( "^" bitor )* ;
bitor          → unary ( "|" unary )* ;
unary          → ( "!" | "-" | "~" | NOT ) unary | call ;
call           → primary ( "(" arguments? ")" | "." IDENTIFIER | "[" expression "]" )* ;
arguments      → expression ( "," expression )* ;
primary        → NUMBER | STRING | "true" | "false" | "(" expression ")" | IDENTIFIER ;
*/

void ac_psr_set_error(ac_parser *parser, ac_token *token, ac_error code, const char *message)
{
    int line = 0;
    if (token)
        line = token->line;
    if (message)
    {
        unsigned int bufSize = 64 + strlen(parser->source_name) + strlen(message);
        char *msg = ac_alloc(bufSize);
        snprintf(msg, bufSize, "Rule '%s': error (line %d): %s", parser->source_name, line, message);
        if (parser->error.message)
            ac_free(parser->error.message);
        parser->error.message = msg;
    }
    parser->error.line = line;
    parser->error.code = code;
}

void ac_psr_reset_error(ac_parser *parser)
{
    if (parser->error.message)
        ac_free(parser->error.message);
    parser->error.code = AC_ERROR_SUCCESS;
    parser->error.line = -1;
}

ac_error ac_psr_get_last_error(ac_parser *parser)
{
    return parser->error.code;
}

char *ac_psr_get_last_error_message(ac_parser *parser)
{
    return parser->error.message;
}

ac_token *ac_psr_getToken(ac_parser *parser, int index)
{
    if (index >= parser->token_count)
        return NULL;
    return parser->tokens[index];
}

int ac_psr_isEof(ac_parser *parser)
{
    return parser->current >= parser->token_count;
}

// get previous token. often used when current token is an operator and a new binary expression is being created.
ac_token *ac_psr_previous_token(ac_parser *parser)
{
    if (parser->current == 0)
    {
        ac_token *token = ac_psr_getToken(parser, 0);
        ac_psr_set_error(parser, token, AC_ERROR_INVALID_SYNTAX, "invalid syntax");
    }
    return parser->tokens[parser->current - 1];
}

ac_token *ac_psr_current_token(ac_parser *parser)
{
    if (ac_psr_isEof(parser))
        return NULL;
    return parser->tokens[parser->current];
}

ac_token *ac_psr_advance(ac_parser *parser)
{
    if (!ac_psr_isEof(parser))
    {
        return parser->tokens[parser->current++];
    }
    return NULL;
}

int ac_psr_check(ac_parser *parser, ac_token_type token_type)
{
    if (ac_psr_isEof(parser))
        return FALSE;
    return parser->tokens[parser->current]->type == token_type;
}

ac_token *ac_psr_consume(ac_parser *parser, ac_token_type token_type, ac_error code, const char *message)
{
    ac_token *token = NULL;
    if (ac_psr_check(parser, token_type))
        token = ac_psr_advance(parser);
    else
        ac_psr_set_error(parser, ac_psr_current_token(parser), code, message);
    return token;
}

// check if current token is one of the provided token types. If it is, advance once (matched token is now previous)
int ac_psr_match(ac_parser *parser, ...)
{
    int match = FALSE;
    va_list args;
    va_start(args, parser);
    for (;;)
    {
        int token_type = va_arg(args, int);
        if (token_type < 0)
            break;
        if (ac_psr_check(parser, token_type))
        {
            ac_psr_advance(parser);
            match = TRUE;
            break;
        }
    }
    va_end(args);
    return match;
}

ac_expr *ac_psr_range(ac_parser *parser)
{
    int match_type = AC_RANGE_MATCH_ALL;
    uint32_t fixed = 0;
    ac_token *ivar = NULL;
    ac_expr *range_start = NULL;
    ac_expr *range_end = NULL;
    ac_expr *condition = NULL;
    if (MATCH(parser, TOKEN_ANY))
        match_type = AC_RANGE_MATCH_ANY;
    if (MATCH(parser, TOKEN_ALL))
        match_type = AC_RANGE_MATCH_ALL;
    else if (MATCH(parser, TOKEN_NUMBER))
    {
        match_type = AC_RANGE_MATCH_FIXED;
        fixed = *(uint32_t *)ac_psr_previous_token(parser)->value;
    } else
    {
        ac_psr_set_error(parser, ac_psr_current_token(parser), AC_ERROR_INVALID_SYNTAX,
            "expected number or quantifiers 'any' or 'all'");
        return NULL;
    }
    if (!ac_psr_consume(parser, TOKEN_IDENTIFIER, AC_ERROR_UNEXPECTED_TOKEN, "expected identifier"))
        return NULL;
    ivar = ac_psr_previous_token(parser);
    if (!ac_psr_consume(parser, TOKEN_IN, AC_ERROR_UNEXPECTED_TOKEN, "unexpected token"))
        return NULL;
    if (!ac_psr_consume(parser, TOKEN_LPAREN, AC_ERROR_UNEXPECTED_TOKEN, "expected '('"))
        return NULL;
    range_start = ac_psr_expression(parser);
    if (!range_start)
        return NULL;
    if (!ac_psr_consume(parser,
        TOKEN_DOT_DOT,
        AC_ERROR_UNEXPECTED_TOKEN,
        "unexpected range specifier '..'"))
        return NULL;
    range_end = ac_psr_expression(parser);
    if (!range_end)
        return NULL;
    if (!ac_psr_consume(parser, TOKEN_RPAREN, AC_ERROR_UNEXPECTED_TOKEN, "expected ')'"))
        return NULL;
    if (!ac_psr_consume(parser, TOKEN_COLON, AC_ERROR_UNEXPECTED_TOKEN, "expected ':'"))
        return NULL;
    if (!ac_psr_consume(parser, TOKEN_LPAREN, AC_ERROR_UNEXPECTED_TOKEN, "expected '('"))
        return NULL;
    condition = ac_psr_expression(parser);
    if (!ac_psr_consume(parser, TOKEN_RPAREN, AC_ERROR_UNEXPECTED_TOKEN, "expected ')'"))
        return NULL;
    return ac_expr_new_range(match_type, fixed, ivar, range_start, range_end, condition);
}

ac_expr *ac_psr_primary(ac_parser *parser)
{
    if (MATCH(parser, TOKEN_FALSE, TOKEN_TRUE, TOKEN_NUMBER, TOKEN_STRING, TOKEN_IDENTIFIER, TOKEN_REGEX))
        return ac_expr_new_literal(ac_psr_previous_token(parser));
    if (MATCH(parser, TOKEN_LPAREN))
    {
        ac_expr *expr = ac_psr_expression(parser);
        if (!ac_psr_consume(parser,
                            TOKEN_RPAREN,
                            AC_ERROR_UNTERMINATED_EXPRESSION,
                            "missing terminating ')'"))
            return NULL;
        return ac_expr_new_grouping(expr);
    }
    if (MATCH(parser, TOKEN_FOR))
    {
        return ac_psr_range(parser);
    }
    ac_psr_set_error(parser, ac_psr_current_token(parser), AC_ERROR_INVALID_SYNTAX, "invalid syntax");
    return NULL;
}


ac_expr *ac_psr_finish_call(ac_parser *parser, ac_expr *callee)
{
    ac_expr *call = ac_expr_new_call(callee, ac_psr_previous_token(parser));
    while (!ac_psr_check(parser, TOKEN_RPAREN))
    {
        ac_expr *arg = ac_psr_expression(parser);
        if (!arg)
        {
            ac_expr_free_expression(call);
            return NULL;
        }
        ac_expr_call_append_argument(call, arg);
        MATCH(parser, TOKEN_COMMA);
    }
    if (!ac_psr_consume(parser,
                        TOKEN_RPAREN,
                        AC_ERROR_UNTERMINATED_EXPRESSION,
                        "incomplete call (expected ')')"))
    {
        ac_expr_free_expression(call);
        return NULL;
    }
    return call;
}

ac_expr *ac_psr_finish_index(ac_parser *parser, ac_expr *object)
{
    ac_token *bracket = NULL;
    ac_expr *expr = ac_psr_expression(parser);
    if (!expr)
        return NULL;
    if ((bracket = ac_psr_consume(parser,
                                  TOKEN_RBRACKET,
                                  AC_ERROR_UNTERMINATED_EXPRESSION,
                                  "bad index (expected ']')")) == NULL)
        return NULL;
    return ac_expr_new_index(object, expr, bracket);
}

ac_expr *ac_psr_finish_field(ac_parser *parser, ac_expr *object)
{
    ac_token *fieldName;
    if ((fieldName = ac_psr_consume(parser,
                                    TOKEN_IDENTIFIER,
                                    AC_ERROR_UNEXPECTED_TOKEN,
                                    "expected identifier")) == NULL)
    {
        return NULL;
    }
    return ac_expr_new_field(object, fieldName);
}

ac_expr *ac_psr_call(ac_parser *parser)
{
    ac_expr *expr = ac_psr_primary(parser);
    if (!expr)
        return NULL;
    while (TRUE)
    {
        ac_expr *secondary = NULL;
        int call = FALSE;
        if (MATCH(parser, TOKEN_LPAREN))
        {
            call = TRUE;
            secondary = ac_psr_finish_call(parser, expr);
        } else if (MATCH(parser, TOKEN_DOT))
            secondary = ac_psr_finish_field(parser, expr);
        else if (MATCH(parser, TOKEN_LBRACKET))
            secondary = ac_psr_finish_index(parser, expr);
        else
            break;
        if (!secondary)
        {
            // call frees callee (expr). avoid double free
            if (!call) ac_expr_free_expression(expr);
            return NULL;
        }
        expr = secondary;
    }
    return expr;
}

ac_expr *ac_psr_unary(ac_parser *parser)
{
    if (MATCH(parser, TOKEN_BANG, TOKEN_BITNOT, TOKEN_MINUS, TOKEN_HASH))
    {
        ac_token *op = ac_psr_previous_token(parser);
        ac_expr *right = ac_psr_unary(parser);
        if (!right)
            return NULL;
        return ac_expr_new_unary(op, right);
    }
    return ac_psr_call(parser);
}

ac_expr *ac_psr_bitor(ac_parser *parser)
{
    ac_expr *expr = ac_psr_unary(parser);
    if (!expr)
        return NULL;
    while (MATCH(parser, TOKEN_PIPE))
    {
        ac_token *op = ac_psr_previous_token(parser);
        ac_expr *right = ac_psr_unary(parser);
        if (!right)
        {
            ac_expr_free_expression(expr);
            return NULL;
        }
        expr = ac_expr_new_binary(expr, op, right);
    }
    return expr;
}

ac_expr *ac_psr_bitxor(ac_parser *parser)
{
    ac_expr *expr = ac_psr_bitor(parser);
    if (!expr)
        return NULL;
    while (MATCH(parser, TOKEN_BITXOR))
    {
        ac_token *op = ac_psr_previous_token(parser);
        ac_expr *right = ac_psr_bitor(parser);
        if (!right)
        {
            ac_expr_free_expression(expr);
            return NULL;
        }
        expr = ac_expr_new_binary(expr, op, right);
    }
    return expr;
}

ac_expr *ac_psr_bitand(ac_parser *parser)
{
    ac_expr *expr = ac_psr_bitxor(parser);
    if (!expr)
        return NULL;
    while (MATCH(parser, TOKEN_BITAND))
    {
        ac_token *op = ac_psr_previous_token(parser);
        ac_expr *right = ac_psr_bitxor(parser);
        if (!right)
        {
            ac_expr_free_expression(expr);
            return NULL;
        }
        expr = ac_expr_new_binary(expr, op, right);
    }
    return expr;
}

ac_expr *ac_psr_bitshift(ac_parser *parser)
{
    ac_expr *expr = ac_psr_bitand(parser);
    if (!expr)
        return NULL;
    while (MATCH(parser, TOKEN_SHL, TOKEN_SHR))
    {
        ac_token *op = ac_psr_previous_token(parser);
        ac_expr *right = ac_psr_bitand(parser);
        if (!right)
        {
            ac_expr_free_expression(expr);
            return NULL;
        }
        expr = ac_expr_new_binary(expr, op, right);
    }
    return expr;
}

ac_expr *ac_psr_factor(ac_parser *parser)
{
    ac_expr *expr = ac_psr_bitshift(parser);
    if (!expr)
        return NULL;
    while (MATCH(parser, TOKEN_MULT, TOKEN_DIV, TOKEN_MOD))
    {
        ac_token *op = ac_psr_previous_token(parser);
        ac_expr *right = ac_psr_bitshift(parser);
        if (!right)
        {
            ac_expr_free_expression(expr);
            return NULL;
        }
        expr = ac_expr_new_binary(expr, op, right);
    }
    return expr;
}

ac_expr *ac_psr_term(ac_parser *parser)
{
    ac_expr *expr = ac_psr_factor(parser);
    if (!expr)
        return NULL;
    while (MATCH(parser, TOKEN_PLUS, TOKEN_MINUS))
    {
        ac_token *op = ac_psr_previous_token(parser);
        ac_expr *right = ac_psr_factor(parser);
        if (!right)
        {
            ac_expr_free_expression(expr);
            return NULL;
        }
        expr = ac_expr_new_binary(expr, op, right);
    }
    return expr;
}

ac_expr *ac_psr_comparison(ac_parser *parser)
{
    ac_expr *expr = ac_psr_term(parser);
    if (!expr)
        return NULL;
    while (MATCH(parser, TOKEN_GREATER, TOKEN_GREATER_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL))
    {
        ac_token *op = ac_psr_previous_token(parser);
        ac_expr *right = ac_psr_term(parser);
        if (!right)
        {
            ac_expr_free_expression(expr);
            return NULL;
        }
        expr = ac_expr_new_binary(expr, op, right);
    }
    return expr;
}

ac_expr *ac_psr_equality(ac_parser *parser)
{
    ac_expr *expr = ac_psr_comparison(parser);
    if (!expr)
        return NULL;
    while (MATCH(parser, TOKEN_EQUAL_EQUAL, TOKEN_BANG_EQUAL, TOKEN_CONTAINS, TOKEN_ICONTAINS,
        TOKEN_STARTSWITH, TOKEN_ISTARTSWITH, TOKEN_ENDSWITH, TOKEN_IENDSWITH, TOKEN_IEQUALS, TOKEN_MATCHES))
    {
        ac_token *op = ac_psr_previous_token(parser);
        ac_expr *right = ac_psr_comparison(parser);
        if (!right)
        {
            ac_expr_free_expression(expr);
            return NULL;
        }
        expr = ac_expr_new_binary(expr, op, right);
    }
    return expr;
}

ac_expr *ac_psr_not(ac_parser *parser)
{
    if (MATCH(parser, TOKEN_NOT))
    {
        ac_token *op = ac_psr_previous_token(parser);
        ac_expr *right = ac_psr_equality(parser);
        if (!right)
            return NULL;
        return ac_expr_new_unary(op, right);
    }
    return ac_psr_equality(parser);
}

ac_expr *ac_psr_or(ac_parser *parser)
{
    ac_expr *expr = ac_psr_not(parser);
    if (!expr)
        return NULL;
    while (MATCH(parser, TOKEN_OR))
    {
        ac_token *op = ac_psr_previous_token(parser);
        ac_expr *right = ac_psr_not(parser);
        if (!right)
        {
            ac_expr_free_expression(expr);
            return NULL;
        }
        expr = ac_expr_new_binary(expr, op, right);
    }
    return expr;
}

ac_expr *ac_psr_and(ac_parser *parser)
{
    ac_expr *expr = ac_psr_or(parser);
    if (!expr)
        return NULL;
    while (MATCH(parser, TOKEN_AND))
    {
        ac_token *op = ac_psr_previous_token(parser);
        ac_expr *right = ac_psr_or(parser);
        if (!right)
        {
            ac_expr_free_expression(expr);
            return NULL;
        }
        expr = ac_expr_new_binary(expr, op, right);
    }
    return expr;
}

ac_expr *ac_psr_expression(ac_parser *parser)
{
    return ac_psr_and(parser);
}

ac_expr *ac_psr_rule_body(ac_parser *parser)
{
    // means we are at the start of a rule scope {}
    if (!ac_psr_consume(parser, TOKEN_LBRACE, AC_ERROR_UNEXPECTED_TOKEN, "expected '{'"))
        return NULL;
    ac_expr *expr = ac_psr_expression(parser);
    if (!expr)
        return NULL;
    if (!ac_psr_consume(parser,
                        TOKEN_RBRACE,
                        AC_ERROR_INVALID_SYNTAX,
                        "invalid syntax"))
        return NULL;
    return expr;
}

ac_statement *ac_psr_rule(ac_parser *parser, int private)
{
    ac_statement *rule = NULL;
    ac_token *event_type = NULL;
    if (!ac_psr_consume(parser, TOKEN_IDENTIFIER, AC_ERROR_UNEXPECTED_TOKEN, "expected rule identifier"))
        return NULL;
    ac_token *id = ac_psr_previous_token(parser);
    if (MATCH(parser, TOKEN_COLON))
    {
        if (!ac_psr_consume(parser, TOKEN_IDENTIFIER, AC_ERROR_UNEXPECTED_TOKEN, "expected event type"))
            return NULL;
        event_type = ac_psr_previous_token(parser);
    }
    ac_expr *condition = ac_psr_rule_body(parser);
    if (!condition)
        return NULL;
    rule = ac_expr_new_rule(id, event_type, condition, FALSE, private);
    return rule;
}

#define SPAN_MINUTE 60

ac_statement *ac_psr_sequence(ac_parser *parser)
{
    ac_statement *seq = NULL;
    uint32_t maxSpan = 0;
    if (!ac_psr_consume(parser, TOKEN_IDENTIFIER, AC_ERROR_UNEXPECTED_TOKEN, "expected sequence identifier"))
        return NULL;
    ac_token *id = ac_psr_previous_token(parser);
    // optional maxSpan (if none or 0 specified, do not do a maxSpan check in VM)
    if (ac_psr_check(parser, TOKEN_COLON))
    {
        ac_psr_advance(parser);
        if (!ac_psr_consume(parser, TOKEN_NUMBER, AC_ERROR_UNEXPECTED_TOKEN, "expected number"))
            return NULL;
        maxSpan = *(uint32_t *) ac_psr_previous_token(parser)->value;
        const char *unit_error = "expected time unit (s = seconds, m = minutes)";
        if (!ac_psr_consume(parser, TOKEN_IDENTIFIER, AC_ERROR_UNEXPECTED_TOKEN, unit_error))
            return NULL;
        ac_token *unit = ac_psr_previous_token(parser);
        char *uval = ac_psr_previous_token(parser)->value;
        if (strlen(uval) != 1 || (uval[0] != 's' && uval[0] != 'm')) // require verbosity with seconds (must have 's')
        {
            ac_psr_set_error(parser, unit, AC_ERROR_UNEXPECTED_TOKEN, unit_error);
            return NULL;
        }
        if (*uval == 'm')
            maxSpan *= SPAN_MINUTE;
    }
    if (!ac_psr_consume(parser, TOKEN_LBRACKET, AC_ERROR_UNEXPECTED_TOKEN, "expected '['"))
        return NULL;
    seq = ac_expr_new_sequence(id, maxSpan);
    // accept rule body '{', scope indicator ':', or rule identifier only. Identifier is checked for type later.
    while (ac_psr_check(parser, TOKEN_IDENTIFIER) || ac_psr_check(parser, TOKEN_LBRACE) || ac_psr_check(
               parser, TOKEN_COLON))
    {
        ac_statement *rule = NULL;
        int isExternRule = FALSE;
        if (ac_psr_check(parser, TOKEN_IDENTIFIER))
            isExternRule = TRUE;
        if (!isExternRule)
        {
            ac_token *event_type = NULL;
            ac_token *ph = ac_psr_current_token(parser);
            if (MATCH(parser, TOKEN_COLON))
            {
                if (!ac_psr_consume(parser, TOKEN_IDENTIFIER, AC_ERROR_UNEXPECTED_TOKEN, "expected event type"))
                {
                    ac_expr_free_sequence(seq);
                    return NULL;
                }
                event_type = ac_psr_previous_token(parser);
            }
            ac_expr *cond = ac_psr_rule_body(parser);
            if (!cond)
            {
                ac_expr_free_sequence(seq);
                return NULL;
            }
            // create with token for debugging
            rule = ac_expr_new_rule(ph, event_type, cond, isExternRule, TRUE);
        } else
        {
            ac_token *name = ac_psr_advance(parser);
            rule = ac_expr_new_rule(name, NULL, NULL, isExternRule, TRUE);
        }
        ac_expr_sequence_append_rule(seq, rule);
        MATCH(parser, TOKEN_COMMA); // advance if comma
    }
    if (!ac_psr_consume(parser,
                        TOKEN_RBRACKET,
                        AC_ERROR_INVALID_SYNTAX,
                        "invalid syntax"))
    {
        ac_expr_free_sequence(seq);
        return NULL;
    }
    return seq;
}
#undef SPAN_MINUTE

ac_statement *ac_psr_import(ac_parser *parser)
{
    if (!ac_psr_consume(parser,
                        TOKEN_IDENTIFIER,
                        AC_ERROR_UNEXPECTED_TOKEN,
                        "expected module name"))
        return NULL;
    return ac_expr_new_import(ac_psr_previous_token(parser));
}

ac_ast *ac_psr_ast(ac_parser *parser)
{
    ac_ast *program = ac_expr_new_ast(parser->source_name);
    while (1)
    {
        ac_statement *statement = NULL;
        if (MATCH(parser, TOKEN_RULE))
            statement = ac_psr_rule(parser, FALSE);
        else if (MATCH(parser, TOKEN_PRIVATE))
        {
            if (!ac_psr_consume(parser, TOKEN_RULE, AC_ERROR_UNEXPECTED_TOKEN, "expected rule"))
                break;
            statement = ac_psr_rule(parser, TRUE);
        }
        else if (MATCH(parser, TOKEN_SEQUENCE))
            statement = ac_psr_sequence(parser);
        else if (MATCH(parser, TOKEN_IMPORT))
            statement = ac_psr_import(parser);
        else if (MATCH(parser, TOKEN_EOF))
            break;
        else
        {
            ac_psr_set_error(parser, ac_psr_current_token(parser), AC_ERROR_UNEXPECTED_TOKEN, "invalid statement");
            break;
        }
        if (statement)
            ac_expr_ast_add_stmt(program, statement);
        else
            break;
    }
    if (parser->error.code != AC_ERROR_SUCCESS)
    {
        ac_expr_free_ast(program);
        return NULL;
    }
    return program;
}

ac_ast *ac_psr_parse(ac_parser *parser)
{
    return ac_psr_ast(parser);
}

ac_parser *ac_psr_new(ac_lexer *lexer)
{
    ac_parser *parser = ac_alloc(sizeof(ac_parser));
    parser->source_name = lexer->source_name;
    parser->token_count = lexer->token_count;
    parser->tokens = lexer->tokens;

    parser->error.code = AC_ERROR_SUCCESS;
    parser->error.line = -1;
    return parser;
}

void ac_psr_free(ac_parser *parser)
{
    ac_free(parser);
}