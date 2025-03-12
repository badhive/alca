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

#ifndef AC_COMPILER_H
#define AC_COMPILER_H

#include <alca/utils.h>
#include <alca/errors.h>
#include <alca/context.h>
#include <alca/bytecode.h>

typedef struct ac_compiler ac_compiler;

typedef struct
{
    ac_error code;
    char *message;
} ac_compiler_error;

typedef struct ac_module_entry ac_module_entry;
typedef struct ac_rule_entry ac_rule_entry;
typedef struct ac_sequence_entry ac_sequence_entry;

struct ac_module_entry
{
    uint32_t ordinal;
    uint32_t version;
    uint32_t lname;
    uint32_t name_offset;
};

struct ac_rule_entry
{
    uint32_t flags;
    uint32_t code_offset;
    uint32_t module_ordinal;
    uint32_t lname;
    uint32_t name_offset;
};

struct ac_sequence_entry
{
    uint32_t flags;
    uint32_t max_span;
    uint32_t rule_count;
    uint32_t lname;
    uint32_t name_offset;
    uint32_t *rule_indices; // list of rule indices for each rule that sequence uses
};

struct ac_compiler
{
    ac_context *ctx;
    ac_builder *builder;

    ac_arena *data_arena; // used to build data section
    ac_arena *code_arena; // used to build code section

    uint32_t nsources;
    uint32_t nrules;
    uint32_t nmodules;
    uint32_t nsequences;

    ac_lexer **sources;
    ac_ast **asts;

    ac_module_entry *module_table;
    ac_rule_entry *rule_table;
    ac_sequence_entry *sequence_table;

    // error info
    uint32_t error_count;
    ac_compiler_error *errors;

    // compiler is locked once AST is built - no more source files can be added
    int locked;
    // compiler is 'done' once code and data sections are built (compile statements)
    int done;
    // lexer warnings
    int silence_warnings;
};

AC_API void ac_compiler_include_module(ac_compiler *compiler, const char *module_name, ac_module_load_callback callback);

AC_API ac_error ac_compiler_add_file(ac_compiler *compiler, const char *filename);

AC_API ac_error ac_compiler_build_ast(ac_compiler *compiler);

AC_API ac_error ac_compiler_check_ast(ac_compiler *compiler);

AC_API ac_error ac_compiler_compile(ac_compiler *compiler, const char *out);

AC_API void ac_compiler_free(ac_compiler *compiler);

void *__ac_compiler_get_code(ac_compiler *compiler);

void *__ac_compiler_get_data(ac_compiler *compiler);

int __ac_compiler_find_rule_idx_by_name(ac_compiler *compiler, char *name);

AC_API ac_compiler *ac_compiler_new();

AC_API void ac_compiler_free(ac_compiler *compiler);

AC_API void ac_compiler_set_silence_warnings(ac_compiler *compiler, int silence_warnings);

#endif //AC_COMPILER_H
