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

/*
 * types.h stores global types that need to be shared internally by components
 * of the library.
 */

#ifndef AC_TYPES_H
#define AC_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifndef NULL
#define NULL (void *)0
#endif

// type declarations for public implementations

typedef enum ac_token_type ac_token_type;
typedef enum ac_expr_type ac_expr_type;
typedef enum ac_stmt_type ac_stmt_type;
typedef struct ac_token ac_token;
typedef struct ac_lexer ac_lexer;
typedef struct ac_parser ac_parser;
typedef struct ac_expr ac_expr;
typedef struct ac_statement ac_statement;
typedef struct ac_ast ac_ast;

typedef int ac_error;
typedef int ac_field_type;

// type definitions

#define TRUE (unsigned char)1
#define FALSE (unsigned char)0

enum ac_token_type
{
    // characters
    TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_DOT_DOT,
    TOKEN_LBRACKET, TOKEN_RBRACKET, TOKEN_COMMA, TOKEN_DOT, TOKEN_PLUS,
    TOKEN_MINUS, TOKEN_DIV, TOKEN_MULT, TOKEN_MOD, TOKEN_PIPE,
    TOKEN_BITAND, TOKEN_BITNOT, TOKEN_BITXOR, TOKEN_EQUAL, TOKEN_COLON,
    TOKEN_HASH,

    // comparisons & operators
    TOKEN_BANG, TOKEN_BANG_EQUAL, TOKEN_EQUAL_EQUAL, TOKEN_GREATER,
    TOKEN_GREATER_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_SHR, TOKEN_SHL, TOKEN_ENDSWITH, TOKEN_IENDSWITH, TOKEN_STARTSWITH,
    TOKEN_ISTARTSWITH, TOKEN_CONTAINS, TOKEN_ICONTAINS, TOKEN_IEQUALS,
    TOKEN_MATCHES,

    // literals
    TOKEN_NUMBER, TOKEN_STRING, TOKEN_REGEX, TOKEN_IDENTIFIER,

    // keywords
    TOKEN_TRUE, TOKEN_FALSE, TOKEN_OR, TOKEN_AND, TOKEN_NOT, TOKEN_RULE,
    TOKEN_SEQUENCE, TOKEN_IMPORT, TOKEN_PRIVATE, TOKEN_FOR,
    TOKEN_ANY, TOKEN_ALL, TOKEN_IN,

    // fake token (for type checking)
    TOKEN_OBJECT,

    TOKEN_EOF
};

enum ac_expr_type
{
    EXPR_BINARY,
    EXPR_GROUPING,
    EXPR_UNARY,
    EXPR_LITERAL,
    EXPR_CALL,
    EXPR_FIELD,
    EXPR_INDEX,
    EXPR_RANGE,
};

enum ac_stmt_type
{
    STMT_RULE,
    STMT_SEQUENCE,
    STMT_IMPORT,
};

#define AC_SEQUENCE_RULE 0x00000001
#define AC_PRIVATE_RULE 0x00000002

#define AC_RANGE_MATCH_ANY 1
#define AC_RANGE_MATCH_ALL 2
#define AC_RANGE_MATCH_FIXED 3

struct ac_lexer
{
    // source file
    const char *source;
    // file name (for errors / debugging)
    const char *source_name;
    size_t source_size;
    ac_token **tokens;
    int token_count;
    // current line number in source file, incremented when \n is hit
    int line;
    // offset of current character in source being scanned
    int current;
    // if last character was whitespace
    int whitespace;
    // silence warnings, particularly encoding issues
    int silence_warnings;
    
    ac_error status;
    char *error_msg;
};

/*
Parser parses tokens into an AST using recursive descent. In other words, grammatical expressions
are created from highest precedence (solved first, e.g. brackets or literals), to lowest
(solved last, e.g. equalities). It produces a binary tree (each branch is one side of the operation, such as ==)
which is passed to the compiler.

One way to determine the validity of a rule AST is checking whether the root node has a comparison, logical operator
(and, or, not), or is a sole identifier (which we can only type check later).
 */
struct ac_parser
{
    const char *source_name;
    ac_token **tokens;
    int token_count;
    int current;

    struct
    {
        int line;
        ac_error code;
        char *message;
    } error;
};

struct ac_token
{
    ac_token_type type;
    void *value;
    int line;
    int flags;
};

struct ac_expr
{
    ac_expr_type type;

    union
    {
        // operations involving two expressions (e.g. comparison, arithmetic)
        struct
        {
            ac_expr *left;
            ac_token *operator;
            ac_expr *right;
            ac_token_type operand_type; // populated by type checker (bool, int, string, arr)
        } binary;

        // operations involving one expression (e.g. bitwise not, logical not)
        struct
        {
            ac_token *operator;
            ac_expr *right;
        } unary;

        struct
        {
            ac_expr *expression;
        } grouping;

        struct
        {
            ac_token *value;
        } literal;

        struct
        {
            ac_field_type return_type;
            ac_expr *callee; // identifier or parentheses-wrapped expression (primary)
            uint32_t arg_count;
            ac_token *paren; // saved for error reporting
            ac_expr **arguments;
        } call;

        struct
        {
            ac_field_type identifier_type;
            ac_expr *object;
            ac_token *field_name;
        } field;

        struct
        {
            ac_field_type item_type;
            ac_expr *object;
            ac_expr *index; // resolves to NUMBER
            ac_token *bracket; // error reporting
        } index;

        struct
        {
            uint32_t any;
            uint32_t all;
            uint32_t fixed;
            ac_token *ivar;
            ac_expr *start;
            ac_expr *end;
            ac_expr *condition;
        } range;
    } u;
};

struct ac_statement
{
    ac_stmt_type type;

    union
    {
        struct
        {
            int external;
            int private;
            ac_token *name;
            ac_token *event;
            ac_expr *condition;
        } rule;

        struct
        {
            ac_token *name;
            uint32_t max_span;
            uint32_t rule_count;
            ac_statement **rules;
        } sequence;

        struct
        {
            ac_token *name;
        } import;
    } u;
};

struct ac_ast
{
    const char *path;
    uint32_t stmt_count;
    ac_statement **statements;
};

#endif //AC_TYPES_H
