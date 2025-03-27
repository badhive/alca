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

#include <alca/arena.h>
#include <alca/checker.h>
#include <alca/context.h>
#include <alca/utils.h>
#include <alca/errors.h>
#include <alca/expr.h>
#include <alca/lexer.h>
#include <alca/parser.h>
#include <alca/bytecode.h>

#include <alca/compiler.h>

/*

File structure (v1)

1. Header

0    1    2    3    4    5    6    7    8
+-------------------+-------------------+
| signature (ALCA)  |      version      |
+-------------------+-------------------+
|    data start     |     data size     |
+-------------------+-------------------+
|    code start     |     code size     |
+-------------------+-------------------+
|   module count    |    rule count     |
+-------------------+-------------------+
|  sequence count   |
+-------------------+

2. Module table (entry)
- Each module in the module table is loaded into the environment via callback.
- Once an event is received with a type matching a module name, the `data` field for that
module is updated with the new event data, rules that use that module's event type are run.
    - a pointer to the event data is passed on the rule's vstack.
- since operations on the module data are all read-only, parallelization considerations can be made.

0    1    2    3    4    5    6    7    8
+-------------------+-------------------+
|  module ordinal   |      version      |
+-------------------+-------------------+
|     name len      |    name offset    |
+-------------------+-------------------+

3. Rule table (entry)
- flags: e.g. private rule flag. If set, do not call rule until another rule / sequence calls it.
- if an incoming event's type does not match the name of the module associated with the rule (ordinal => name),
then the rule isn't run.

0    1    2    3    4    5    6    7    8
+-------------------+-------------------+
|       flags       |    code start     |
+-------------------+-------------------+
|  module ordinal   |    name length    |
+-------------------+-------------------+
|    name offset    |
+-------------------+

4. Sequence table (entry)
- each index is the index of the rule in the rule table. The loader uses this information
create and monitor the sequence state.
- when a rule within a sequence is evaluated and returns true, the 'last true eval time' for that specific rule is
updated in the sequence state. If the last rule in the sequence is triggered, a check is done to see whether:
    1. All previous rules triggered within a timeframe of `max_span`
    2. The trigger times for the previous rules increase with the rule indices
If both conditions are satisfied, the sequence returns true.

0    1    2    3    4    5    6    7    8
+-------------------+-------------------+
|       flags       |     max_span      |
+-------------------+-------------------+
|    rule count     |    name length    |
+-------------------+-------------------+
|    name offset    |
+-------------------+-------------------+
|            rule index list            |
|                  ...                  |
+---------------------------------------+

5. Data section. Contiguous block of memory containing strings and objects used by the code.
Strings and objects are referenced by memory. 0xFFFFFFFF is the invalid address value, and may be used by
sensors to show that a string / object field is omitted.

6. Code section, where actual VM instructions for the rules reside. Each run of code would result in a 0 or 1
remaining on the stack, which would be the boolean result of the operation.

*/

#define ALCA_HEADER_SIZE (4 * 7)
#define ALCA_MODULE_ENTRY_SIZE (4 * 4)
#define ALCA_RULE_ENTRY_SIZE (4 * 6)
#define ALCA_SEQUENCE_ENTRY_HEADER_SIZE (4 * 5)

ac_compiler *ac_compiler_new()
{
    ac_compiler *compiler = ac_alloc(sizeof(ac_compiler));
    compiler->data_arena = ac_arena_create(0);
    compiler->code_arena = ac_arena_create(0);
    compiler->ctx = ac_context_new();

    compiler->builder = ac_alloc(sizeof(ac_builder));
    compiler->builder->code = compiler->code_arena;
    compiler->builder->data = compiler->data_arena;
    compiler->builder->ctx = compiler->ctx;
    return compiler;
}

void ac_compiler_free(ac_compiler *compiler)
{
    if (compiler->sources)
    {
        for (int i = 0; i < compiler->nsources; i++)
            ac_lex_free(compiler->sources[i]);
        ac_free(compiler->sources);
    }
    if (compiler->sequence_table)
    {
        for (int i = 0; i < compiler->nsequences; i++)
        {
            if (compiler->sequence_table->rule_indices)
                ac_free(compiler->sequence_table->rule_indices);
        }
        ac_free(compiler->sequence_table);
    }
    if (compiler->asts)
    {
        for (int i = 0; i < compiler->nsources; i++)
            ac_expr_free_ast(compiler->asts[i]);
        ac_free(compiler->asts);
    }
    if (compiler->module_table) ac_free(compiler->module_table);
    if (compiler->rule_table) ac_free(compiler->rule_table);
    ac_arena_destroy(compiler->code_arena);
    ac_arena_destroy(compiler->data_arena);
    ac_context_free(compiler->ctx);
    ac_free(compiler->builder);
    ac_free(compiler);
}

void ac_compiler_set_silence_warnings(ac_compiler *compiler, int silence_warnings)
{
    compiler->silence_warnings = silence_warnings;
}

void compiler_add_error(ac_compiler *compiler, ac_error error, char *message)
{
    if (compiler->error_count == 0)
        compiler->errors = ac_alloc(sizeof(ac_compiler_error));
    else
        compiler->errors = ac_realloc(compiler->errors, (compiler->error_count + 1) * sizeof(ac_compiler_error));
    memcpy(
        compiler->errors + compiler->error_count,
        &(ac_compiler_error){error, message},
        sizeof(ac_compiler_error)
    );
    compiler->error_count++;
}

void compiler_add_source_lexer(ac_compiler *compiler, ac_lexer *lexer)
{
    if (compiler->nsources == 0)
        compiler->sources = ac_alloc(sizeof(ac_lexer *));
    else
        compiler->sources = ac_realloc(compiler->sources, (compiler->nsources + 1) * sizeof(ac_lexer *));

    compiler->sources[compiler->nsources++] = lexer;
}

void compiler_add_module_entry(ac_compiler *compiler, ac_module_entry *entry)
{
    if (compiler->nmodules == 0)
        compiler->module_table = ac_alloc(sizeof(ac_module_entry));
    else
        compiler->module_table = ac_realloc(compiler->module_table, (compiler->nmodules + 1) * sizeof(ac_module_entry));

    memcpy(compiler->module_table + compiler->nmodules++, entry, sizeof(ac_module_entry));
}

int compiler_find_module_ordinal(ac_compiler *compiler, char *module_name, uint32_t *ordinal)
{
    if (!module_name)
    {
        *ordinal = 0;
        return ERROR_SUCCESS;
    }
    ac_module_entry *entry = NULL;
    for (int i = 0; i < compiler->nmodules; i++)
    {
        entry = compiler->module_table + i;
        char *name = ac_arena_get_string(compiler->data_arena, entry->name_offset);
        if (!name)
            continue;
        if (strncmp(name, module_name, entry->lname) == 0)
        {
            *ordinal = entry->ordinal;
            return ERROR_SUCCESS;
        }
    }
    return ERROR_UNSUCCESSFUL;
}

uint32_t compiler_add_rule_entry(ac_compiler *compiler, ac_rule_entry *entry)
{
    uint32_t idx = compiler->nrules;
    if (compiler->nrules == 0)
        compiler->rule_table = ac_alloc(sizeof(ac_rule_entry));
    else
        compiler->rule_table = ac_realloc(compiler->rule_table, (compiler->nrules + 1) * sizeof(ac_rule_entry));

    memcpy(compiler->rule_table + compiler->nrules++, entry, sizeof(ac_rule_entry));
    return idx;
}

void compiler_add_sequence_entry(ac_compiler *compiler, ac_sequence_entry *entry)
{
    if (compiler->nsequences == 0)
        compiler->sequence_table = ac_alloc(sizeof(ac_sequence_entry));
    else
        compiler->sequence_table = ac_realloc(compiler->sequence_table, (compiler->nsequences + 1) * sizeof(ac_sequence_entry));

    memcpy(compiler->sequence_table + compiler->nsequences++, entry, sizeof(ac_sequence_entry));
}

int __ac_compiler_find_rule_idx_by_name(ac_compiler *compiler, char *name)
{
    for (int i = 0; i < compiler->nrules; i++)
    {
        if (!compiler->rule_table)
            return -1;
        ac_rule_entry entry = compiler->rule_table[i];
        char *rname = ac_arena_get_string(compiler->data_arena, entry.name_offset);
        if (rname) {
            if (strcmp(rname, name) == 0)
                return i;
        }
    }
    return -1;
}

// INTERNAL
void compiler_add_ast(ac_compiler *compiler, ac_ast *ast, int src_idx)
{
    if (compiler->asts == NULL)
        compiler->asts = ac_alloc(compiler->nsources * sizeof(ac_ast *));
    compiler->asts[src_idx] = ast;
}

ac_error ac_compiler_add_file(ac_compiler *compiler, const char *filename)
{
    char *buffer = NULL;
    ac_error status = ERROR_SUCCESS;
    uint32_t file_size = 0;
    ac_lexer *lexer;
    if (compiler->locked)
        return ERROR_COMPILER_LOCKED;
    if (compiler->done)
        return ERROR_COMPILER_DONE;
    if ((status = ac_read_file(filename, &buffer, &file_size)) != ERROR_SUCCESS)
        return status;
    lexer = ac_lex_new(buffer, filename, file_size);
    ac_lex_set_silence_warnings(lexer, compiler->silence_warnings);
    if (!ac_lex_scan(lexer))
    {
        compiler_add_error(compiler, lexer->status, lexer->error_msg);
        ac_lex_free(lexer);
        return ERROR_UNSUCCESSFUL;
    }
    compiler_add_source_lexer(compiler, lexer);
    return ERROR_SUCCESS;
}

ac_error ac_compiler_build_ast(ac_compiler *compiler)
{
    if (compiler->locked)
        return ERROR_COMPILER_LOCKED;
    ac_ast **asts = ac_alloc(compiler->nsources * sizeof(ac_ast *));
    ac_lexer *lexer = NULL;
    ac_parser *parser = NULL;
    ac_ast *ast = NULL;
    for (int i = 0; i < compiler->nsources; i++)
    {
        lexer = compiler->sources[i];
        parser = ac_psr_new(lexer);
        ast = ac_psr_parse(parser);
        if (parser->error.code != ERROR_SUCCESS)
        {
            compiler_add_error(compiler, parser->error.code, parser->error.message);
            ac_psr_free(parser);
            ac_free(asts);
            return ERROR_UNSUCCESSFUL;
        }
        ac_psr_free(parser);
        asts[i] = ast;
    }
    for (int i = 0; i < compiler->nsources; i++)
        compiler_add_ast(compiler, asts[i], i);
    ac_free(asts);
    compiler->locked = TRUE;
    return ERROR_SUCCESS;
}

// called for every default module before compiling
void ac_compiler_include_module(ac_compiler *compiler, const char *module_name, ac_module_load_callback callback)
{
    if (compiler->locked || compiler->done)
        return;
    ac_context_add_module_load_callback(compiler->ctx, module_name, callback);
}

ac_error ac_compiler_check_ast(ac_compiler *compiler)
{
    if (!compiler->asts)
        return ERROR_NOT_PARSED;
    if (compiler->done)
        return ERROR_COMPILER_DONE;
    ac_error ret = ERROR_SUCCESS;
    if (!compiler->ctx)
        compiler->ctx = ac_context_new();
    for (int i = 0; i < compiler->nsources; i++)
    {
        ac_ast *ast = compiler->asts[i];
        ac_checker *checker = ac_checker_new(ast, compiler->ctx);
        ret = ac_checker_check(checker) ? ERROR_SUCCESS : ERROR_UNSUCCESSFUL;
        int line = 0;
        ac_error code = ERROR_SUCCESS;
        char *err = NULL;
        while (ac_checker_iter_errors(checker, &line, &code, &err))
        {
            char *nerr = ac_alloc(strlen(err) + 1);
            strcpy(nerr, err);
            compiler_add_error(compiler, code, nerr);
        }
        ac_checker_free(checker);
    }
    return ret;
}

ac_error compiler_compile_import(ac_compiler *compiler, ac_statement *import)
{
    // add entry if module hasn't been added yet
    char *name = import->u.import.name->value;
    if (ac_arena_find_string(compiler->data_arena, name) == -1)
    {
        uint32_t lname = strlen(name);
        uint32_t offset = ac_arena_add_string(compiler->data_arena, name, lname);
        ac_module_load_callback cb = NULL;
        if (!ac_context_get_module(compiler->ctx, name, &cb))
            return ERROR_MODULE;
        uint32_t version = ac_context_object_get_module_version(cb());
        uint32_t ordinal = compiler->nmodules + 1; // starts from 1
        ac_module_entry module_entry = {ordinal, version, lname, offset};
        compiler_add_module_entry(compiler, &module_entry);
    }
    return ERROR_SUCCESS;
}

ac_error compiler_compile_rule(ac_compiler *compiler, ac_statement *rule, int seq_rule, uint32_t *idx)
{
    ac_error status = ERROR_SUCCESS;
    uint32_t flags = 0;
    uint32_t lname = 0;
    uint32_t offset = 0;
    uint32_t ordinal = 0;
    uint32_t code_start = ac_arena_size(compiler->code_arena);
    if (!seq_rule)
    {
        lname = strlen(rule->u.rule.name->value);
        offset = ac_arena_add_string(compiler->data_arena, rule->u.rule.name->value, lname);
    }
    if (rule->u.rule.is_private)
        flags |= AC_PRIVATE_RULE;
    if (seq_rule)
        flags |= AC_SEQUENCE_RULE;
    if (rule->u.rule.event)
    {
        if (compiler_find_module_ordinal(compiler, rule->u.rule.event->value, &ordinal) != ERROR_SUCCESS)
            return ERROR_UNSUCCESSFUL;
    }
    if ((status = ac_bytecode_emit_rule(compiler->builder, rule)) != ERROR_SUCCESS)
        return status;
    ac_rule_entry rule_entry = {flags, code_start, ordinal, lname, offset};
    uint32_t entry = compiler_add_rule_entry(compiler, &rule_entry);
    if (idx)
        *idx = entry;
    return ERROR_SUCCESS;
}

ac_error compiler_compile_sequence(ac_compiler *compiler, ac_statement *sequence)
{
    uint32_t flags = 0;
    uint32_t max_span = sequence->u.sequence.max_span;
    uint32_t lname = strlen(sequence->u.sequence.name->value);
    uint32_t offset = ac_arena_add_string(compiler->data_arena,
        sequence->u.sequence.name->value,
        strlen(sequence->u.sequence.name->value));
    uint32_t rule_count = sequence->u.sequence.rule_count;
    uint32_t *indices = ac_alloc(rule_count * sizeof(uint32_t));
    for (int i = 0; i < sequence->u.sequence.rule_count; i++)
    {
        if (sequence->u.sequence.rules[i]->u.rule.external)
        {
            int idx = __ac_compiler_find_rule_idx_by_name(compiler, sequence->u.sequence.rules[i]->u.rule.name->value);
            if (idx == -1)
            {
                ac_free(indices);
                return ERROR_UNSUCCESSFUL;
            }
            indices[i] = (uint32_t)idx;
        } else
        {
            ac_error err = compiler_compile_rule(compiler, sequence->u.sequence.rules[i], TRUE, &indices[i]);
            if (err != ERROR_SUCCESS)
                return err;
        }
    }
    ac_sequence_entry entry = {flags, max_span, rule_count, lname, offset, indices};
    compiler_add_sequence_entry(compiler, &entry);
    return ERROR_SUCCESS;
}

ac_error compiler_export(ac_compiler *compiler, const char *out)
{
    ac_arena *file = ac_arena_create(0);
    if (compiler->module_table)
    {
        for (int i = 0; i < compiler->nmodules; i++)
        {
            ac_module_entry module_entry = compiler->module_table[i];
            ac_arena_add_uint32(file, module_entry.ordinal);
            ac_arena_add_uint32(file, module_entry.version);
            ac_arena_add_uint32(file, module_entry.lname);
            ac_arena_add_uint32(file, module_entry.name_offset);
        }
    }
    if (compiler->rule_table)
    {
        for (int i = 0; i < compiler->nrules; i++)
        {
            ac_rule_entry rule_entry = compiler->rule_table[i];
            ac_arena_add_uint32(file, rule_entry.flags);
            ac_arena_add_uint32(file, rule_entry.code_offset);
            ac_arena_add_uint32(file, rule_entry.module_ordinal);
            ac_arena_add_uint32(file, rule_entry.lname);
            ac_arena_add_uint32(file, rule_entry.name_offset);
        }
    }
    if (compiler->sequence_table)
    {
        for (int i = 0; i < compiler->nsequences; i++)
        {
            ac_sequence_entry sequence_entry = compiler->sequence_table[i];
            ac_arena_add_uint32(file, sequence_entry.flags);
            ac_arena_add_uint32(file, sequence_entry.max_span);
            ac_arena_add_uint32(file, sequence_entry.rule_count);
            ac_arena_add_uint32(file, sequence_entry.lname);
            ac_arena_add_uint32(file, sequence_entry.name_offset);
            for (int j = 0; j < sequence_entry.rule_count; j++)
                ac_arena_add_uint32(file, sequence_entry.rule_indices[j]);
        }
    }
    uint32_t data_size = ac_arena_size(compiler->data_arena);
    uint32_t code_size = ac_arena_size(compiler->code_arena);
    uint32_t data_offset = ac_arena_add_bytes(file, ac_arena_data(compiler->data_arena), data_size);
    uint32_t code_offset = ac_arena_add_bytes(file, ac_arena_data(compiler->code_arena), code_size);

    // header
    ac_arena *hdr = ac_arena_create(0);
    ac_arena_add_uint32(hdr, ALCA_MAGIC);
    ac_arena_add_uint32(hdr, ALCA_VERSION);
    ac_arena_add_uint32(hdr, data_offset);
    ac_arena_add_uint32(hdr, data_size);
    ac_arena_add_uint32(hdr, code_offset);
    ac_arena_add_uint32(hdr, code_size);
    ac_arena_add_uint32(hdr, compiler->nmodules);
    ac_arena_add_uint32(hdr, compiler->nrules);
    ac_arena_add_uint32(hdr, compiler->nsequences);

    ac_arena_prepend_bytes(file, ac_arena_data(hdr), ac_arena_size(hdr));
    ac_arena_destroy(hdr);

    FILE *outfile = fopen(out, "wb");
    if (outfile == NULL)
        return ERROR_COMPILER_EXPORT;
    size_t written = fwrite(ac_arena_data(file), 1, ac_arena_size(file), outfile);
    fclose(outfile);
    if (written != ac_arena_size(file))
    {
        return ERROR_COMPILER_EXPORT;
    }
    return ERROR_SUCCESS;
}

ac_error compiler_compile_statements(ac_compiler *compiler)
{
    // top-level ast
    if (!compiler->asts)
        return ERROR_NOT_PARSED;
    if (compiler->done)
        return ERROR_COMPILER_DONE;
    // it's done once this func has been called. might as well create a new compiler if
    // theres anything else left to compile
    compiler->done = TRUE;
    ac_ast *ast = ac_alloc(sizeof(ac_ast));
    ast->path = "(namespace)";

    for (int i = 0; i < compiler->nsources; i++)
    {
        ac_ast *subast = compiler->asts[i];
        for (int j = 0; j < subast->stmt_count; j++)
            ac_expr_ast_add_stmt(ast, subast->statements[j]);
    }

    for (int i = 0; i < ast->stmt_count; i++)
    {
        ac_statement *stmt = ast->statements[i];
        ac_error code = ERROR_SUCCESS;
        switch (stmt->type)
        {
            case STMT_IMPORT: code = compiler_compile_import(compiler, stmt);
                break;
            case STMT_RULE: code = compiler_compile_rule(compiler, stmt, FALSE, NULL);
                break;
            case STMT_SEQUENCE: code = compiler_compile_sequence(compiler, stmt);
                break;
        }
        if (code != ERROR_SUCCESS)
            return code;
    }
    return ERROR_SUCCESS;
}

ac_error ac_compiler_compile(ac_compiler *compiler, const char *out)
{
    if (compiler->done)
        return ERROR_COMPILER_DONE;
    if (compiler->nsources == 0)
        return ERROR_COMPILER_NO_SOURCE;
    ac_error err;
    err = ac_compiler_build_ast(compiler);
    if (err != ERROR_SUCCESS)
        return err;
    err = ac_compiler_check_ast(compiler);
    if (err != ERROR_SUCCESS)
        return err;
    err = compiler_compile_statements(compiler);
    if (err != ERROR_SUCCESS)
        return err;
    if (out)
    {
        err = compiler_export(compiler, out);
        if (err != ERROR_SUCCESS)
            return err;
    }
    return ERROR_SUCCESS;
}

void *__ac_compiler_get_code(ac_compiler *compiler)
{
    return compiler->code_arena;
}

void *__ac_compiler_get_data(ac_compiler *compiler)
{
    return compiler->data_arena;
}
