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

#ifndef AC_BYTECODE_H
#define AC_BYTECODE_H

#include <alca/arena.h>
#include <alca/context.h>

#define OP_HLT 254

#define OP_ADD 1
#define OP_SUB 2
#define OP_MUL 3
#define OP_DIV 4
#define OP_MOD 5
#define OP_SHL 6
#define OP_SHR 7

#define OP_AND 8
#define OP_OR 9
#define OP_NOT 11
#define OP_XOR 10

#define OP_GT 12
#define OP_LT 13
#define OP_GTE 14
#define OP_LTE 15

#define OP_INTEQ 16
#define OP_STREQ 17
#define OP_BOOLEQ 18
#define OP_INTNE 19
#define OP_STRNE 20
#define OP_BOOLNE 21
// logical
#define OP_ANDL 22
#define OP_ORL 23
#define OP_NOTL 24

#define OP_NEG 25

// control flow
#define OP_JFALSE 26
#define OP_JTRUE 27
#define OP_JMP 28

// stack operations
#define OP_PUSHINT 100 // operand: uint literal
#define OP_PUSHBOOL 101 // operand: float literal
#define OP_PUSHSTRING 102 // operand: string offset
#define OP_PUSHMODULE 103 // operand: module name offset. pushes module object from ctx onto stack
#define OP_OBJECT 104 // replaces context object with its embedded ac_object (populated by event decoder)
// string operations
#define OP_CONTAINS 105
#define OP_ICONTAINS 106
#define OP_STARTSWITH 107
#define OP_ISTARTSWITH 108
#define OP_ENDSWITH 109
#define OP_IENDSWITH 110
#define OP_IEQUALS 111
#define OP_MATCHES 112
#define OP_STRLEN 113

#define OP_RULE 200 // operand: rule name offset
#define OP_CALL 201
#define OP_FIELD 202 // operand: field name offset
#define OP_INDEX 203 // operand: index number
#define OP_PUSH 204
#define OP_POP 205
// accumulator access
#define OP_LOAD 206
#define OP_STORE 207

typedef struct ac_builder
{
    char *module_name;
    char *iter;
    ac_arena *code;
    ac_arena *data;
    ac_context *ctx;
} ac_builder;

ac_error ac_bytecode_emit_rule(ac_builder *builder, ac_statement *rule);

#endif //AC_BYTECODE_H
