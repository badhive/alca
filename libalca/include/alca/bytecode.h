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
