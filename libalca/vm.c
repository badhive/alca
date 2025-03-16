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

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <alca/arena.h>
#include <alca/utils.h>
#include <alca/vm.h>

#include <pcre2.h>
#include "hashmap.h"

#define VM_STACK_MAX 256
#define VM_ACCUM_MAX 4
#define VM_MAX_CALLBACKS 10

#if(ALCA_BUILD_DEBUG == 1)
#define DBGPRINT(...) vm_debug_print(__VA_ARGS__)
#else
#define DBGPRINT(...)
#endif

#define push(x) {\
    if (vm->sp >= VM_STACK_MAX) \
    { \
        *result = ERROR_STACK_OVERFLOW; \
        stop = TRUE; \
        break; \
    } \
    vm->stack[vm->sp++] = x; \
}

#define pop(x) {\
    assert(vm->sp > 0); \
    x = vm->stack[--vm->sp]; \
}

#define pushcall(x) {\
    if (vm->csp >= VM_STACK_MAX) \
    { \
        *result = ERROR_MAX_CALLS; \
        stop = TRUE; \
        break; \
    } \
    vm->callstack[vm->csp++] = x; \
}

#define popcall(x) {\
    assert(vm->csp > 0); \
    x = vm->callstack[--vm->csp]; \
}

#define arg(x) \
    uint32_t x = *(uint32_t *)(vm->ip); \
    vm->ip += sizeof(x); \

typedef struct modinfo
{
    uint32_t ordinal;
    char *name;
} modinfo;

typedef struct vm_monrule
{
    uint32_t idx;
    char *name;
    time_t trigger;
} vm_monrule;

typedef struct vm_sequence
{
    int flag;
    uint32_t max_span;
    uint32_t rule_count;
    uint32_t *rule_indices;
    struct hashmap *monitored_rules;
} vm_sequence;

struct ac_vm
{
    uint32_t sp; // stack ptr
    uint32_t csp; // call stack ptr
    unsigned char *ip; // inst ptr
    char *current_rule;

    ac_object stack[VM_STACK_MAX];
    long long callstack[VM_STACK_MAX];

    uint32_t code_size;
    unsigned char *code;
    ac_arena *data;

    vm_sequence *sequence_table;

    struct hashmap *modules;

    uint32_t ncb;
    int ntriggers;
    ac_trigger_callback cb[VM_MAX_CALLBACKS];
    ac_compiler *cpl;
};
ac_error ac_vm_exec_code(ac_vm *vm, unsigned char *code, uint32_t *result);

char *lowercase(char *s)
{
    char *x = ac_alloc(strlen(s) + 1);
    strcpy(x, s);
    for(int i = 0; x[i]; i++) \
        x[i] = (char)tolower(x[i]);
    return x;
}

int modinfo_cmp(const void *a, const void *b, void *c)
{
    const modinfo *obja = a;
    const modinfo *objb = b;
    return obja->ordinal != objb->ordinal;
}

uint64_t modinfo_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const modinfo *obj = item;
    return hashmap_murmur(&obj->ordinal, sizeof(obj->ordinal), seed0, seed1);
}

int monrule_cmp(const void *a, const void *b, void *c)
{
    const vm_monrule *obja = a;
    const vm_monrule *objb = b;
    return obja->idx != objb->idx;
}

uint64_t monrule_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const vm_monrule *obj = item;
    return hashmap_murmur(&obj->idx, sizeof(obj->idx), seed0, seed1);
}

void vm_debug_print(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
}

ac_vm *ac_vm_new(ac_compiler *compiler)
{
    ac_vm *vm = ac_alloc(sizeof(ac_vm));
    vm->cpl = compiler;
    vm->sp = 0;
    vm->ip = 0;
    vm->code_size = ac_arena_size(compiler->code_arena);
    vm->code = ac_arena_data(compiler->code_arena);
    vm->data = compiler->data_arena;

    // load all module objects into context
    ac_context_load_modules(vm->cpl->ctx);

    // construct sequence table
    vm->sequence_table = ac_alloc(compiler->nsequences * sizeof(vm_sequence));
    for (int i = 0; i < compiler->nsequences; i++)
    {
        ac_sequence_entry *entry = &compiler->sequence_table[i];
        vm->sequence_table[i].monitored_rules = hashmap_new(
            sizeof(vm_monrule),
            0,
            0,
            0,
            monrule_hash,
            monrule_cmp,
            NULL,
            NULL);
        vm->sequence_table[i].max_span = entry->max_span;
        vm->sequence_table[i].rule_count = entry->rule_count;
        vm->sequence_table[i].rule_indices = entry->rule_indices;
        for (uint32_t j = 0; j < entry->rule_count; j++)
        {
            hashmap_set(vm->sequence_table[i].monitored_rules, &(vm_monrule){entry->rule_indices[j], 0});
        }
    }
    // create mapping of module ordinals to event type names
    vm->modules = hashmap_new(
        sizeof(modinfo),
        0,
        0,
        0,
        modinfo_hash,
        modinfo_cmp,
        NULL,
        NULL);
    for (int i = 0; i < compiler->nmodules; i++)
    {
        char *modname = ac_arena_get_string(compiler->data_arena, compiler->module_table[i].name_offset);
        hashmap_set(vm->modules, &(modinfo){compiler->module_table[i].ordinal, modname});
    }
    return vm;
}

int ac_vm_add_trigger_callback(ac_vm *vm, ac_trigger_callback cb)
{
    if (vm->ncb >= VM_MAX_CALLBACKS)
        return FALSE;
    vm->cb[vm->ncb++] = cb;
    return TRUE;
}

void vm_report_triggered(ac_vm *vm, int type, char *name, time_t at, void *exec_context)
{
    for (int i = 0; i < vm->ncb; i++)
    {
        if (vm->cb[i])
            vm->cb[i](type, name, at, exec_context);
    }
    vm->ntriggers++;
}

// notifies sequences monitoring rule at `rule_idx` about the time the rule was triggered.
// if the trigger times are ascending, then rules triggered sequentially so sequence is TRUE
void vm_update_sequences(ac_vm *vm, uint32_t rule_index, time_t trigger_time, void *exec_context)
{
    time_t at = time(NULL);
    vm_monrule rule = {rule_index, vm->current_rule, trigger_time};
    for (int i = 0; i < vm->cpl->nsequences; i++)
    {
        vm_sequence *s = &vm->sequence_table[i];
        if (hashmap_get(s->monitored_rules, &rule)) // if sequence is monitoring this rule
            hashmap_set(s->monitored_rules, &rule);
        else
            continue;
        int idx = 0;
        time_t first = 0;
        time_t last = -1;
        int sequential = TRUE;
        for (int j = 0; j < s->rule_count; j++) // iter starts
        {
            const vm_monrule *r = hashmap_get(s->monitored_rules, &(vm_monrule){s->rule_indices[j]});
            if (!r->trigger || (r->trigger && r->trigger < last))
            {
                sequential = FALSE;
                break;
            }
            if (idx == 0)
                first = r->trigger;
            last = r->trigger;
            idx++;
        }
        if (sequential)
        {
            // if maxSpan is 0, or if it's greater than 0 AND the trigger times are within the span window
            if (!s->max_span || (s->max_span && last - first <= s->max_span))
            {
                char *name = ac_arena_get_string(vm->data, vm->cpl->sequence_table[i].name_offset);
                vm_report_triggered(vm, AC_VM_SEQUENCE, name, at, exec_context);
            }
        }
    }
}

int ac_vm_get_trigger_count(ac_vm *vm)
{
    return vm->ntriggers;
}

/*
+----------+
| version  | module version
+----------+
| etypelen | event type length
+----------+
| typename | event type
+----------+
| evntdata | event data
+----------+
*/

// executes all rules with matching event type or event type 0 (none)
ac_error ac_vm_exec(ac_vm *vm, unsigned char *event, uint32_t esize, void *exec_context)
{
    vm->ntriggers = 0; // reset trigger count
    char *event_type = NULL;
    ac_error err = ERROR_SUCCESS;

    // get event version, name and data
    if (esize < 8)
        return ERROR_BAD_DATA;
    uint32_t etypever = b2l(*(uint32_t *)event);
    uint32_t etypelen = b2l(*(uint32_t *)(event + sizeof(uint32_t)));
    if (esize + 4 < etypelen)
        return ERROR_BAD_DATA;
    event_type = (char *) event + (2 * sizeof(uint32_t));

    ac_context_object *mod = ac_context_get(vm->cpl->ctx, event_type);
    if (!mod)
        return ERROR_MODULE; // unknown module name
    if (ac_context_object_get_module_version(mod) != etypever)
        return ERROR_MODULE_VERSION; // version mismatch
    if (!ac_context_object_unmarshal_evtdata(mod, (unsigned char *) event_type + etypelen))
        return ERROR_BAD_DATA;

    for (int i = 0; i < vm->cpl->nrules; i++)
    {
        ac_rule_entry *entry = &vm->cpl->rule_table[i];
        if (entry->module_ordinal)
        {
            const modinfo *mi = hashmap_get(vm->modules, &(modinfo){entry->module_ordinal});
            if (mi == NULL) continue;
            if (strncmp(mi->name, event_type, etypelen) == 0)
            {
                uint32_t result = 0;
                vm->current_rule = ac_arena_get_string(vm->data, entry->name_offset);
                ac_error e = ac_vm_exec_code(vm, vm->code + entry->code_offset, &result);
                if (e != ERROR_SUCCESS)
                {
                    printf("[acvm] Rule %s: caught exception: code = %d\n", vm->current_rule, e);
                    err = e;
                }
                if (result)
                {
                    time_t at = time(NULL);
                    // report rule has been triggered only if it is not anonymous or private
                    if (!(entry->flags & AC_SEQUENCE_RULE || entry->flags & AC_PRIVATE_RULE))
                        vm_report_triggered(vm, AC_VM_RULE, vm->current_rule, at, exec_context);
                    // done for all rules to account for external (public and private) rules
                    vm_update_sequences(vm, i, at, exec_context);
                }
                vm->current_rule = NULL;
            }
        }
    }
    ac_context_object_clear_module_data(mod);
    return err;
}

// don't free compiler, freed by ac_compiler_free
void ac_vm_free(ac_vm *vm)
{
    if (vm->sequence_table)
    {
        for (int i = 0; i < vm->cpl->nsequences; i++)
        {
            if (vm->sequence_table[i].monitored_rules)
                hashmap_free(vm->sequence_table[i].monitored_rules);
        }
        ac_free(vm->sequence_table);
    }
    ac_free(vm);
}

// executes rule and returns boolean result on OP_HLT (true / false) or exception (false).
// This is the interpreter.
ac_error ac_vm_exec_code(ac_vm *vm, unsigned char *code, uint32_t *result)
{
    int stop = FALSE;
    ac_error err = ERROR_SUCCESS;

    // registers
    ac_object r1;
    ac_object r2;

    // accumulators (persistent storage)
    ac_object regs[VM_ACCUM_MAX] = {0};

    vm->ip = code;
    for (int i = 0; i < vm->csp; i++)
    {
        if (vm->callstack[i] == (long long)vm->ip)
        {
            err = ERROR_RECURSION;
            printf("[acvm] Rule %s: caught exception: code = %d\n", vm->current_rule, err);
            stop = TRUE;
            break;
        }
    }
    while (!stop)
    {
        unsigned char op = *vm->ip++;
        switch (op)
        {
            case OP_HLT:
            {
                pop(r1)
                DBGPRINT("HLT: result = %s", r1.b ? "true" : "false");
                stop = TRUE;
                *result = r1.b;
            }
            break;
            case OP_ADD:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("ADD: %d + %d", r2.i, r1.i);
                r1.i = r1.i + r2.i;
                push(r1)
            }
            break;
            case OP_SUB:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("SUB: %d - %d", r2.i, r1.i);
                r1.i = r2.i - r1.i;
                push(r1)
            }
            break;
            case OP_MUL:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("MUL: %d * %d", r2.i, r1.i);
                r1.i = r1.i * r2.i;
                push(r1)
            }
            break;
            case OP_DIV:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("DIV: %d / %d", r2.i, r1.i);
                r1.i = r2.i / r1.i;
                push(r1)
            }
            break;
            case OP_MOD:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("MOD: %d %% %d", r2.i, r1.i);
                r1.i = r2.i % r1.i;
                push(r1)
            }
            break;
            case OP_SHL:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("SHL: %d << %d", r2.i, r1.i);
                if (r1.i > 32)
                {
                    err = ERROR_BAD_OPERAND;
                    stop = TRUE;
                }
                else
                {
                    r1.i = r2.i << r1.i;
                    push(r1)
                }
            }
            break;
            case OP_SHR:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("SHR: %d >> %d", r2.i, r1.i);
                if (r1.i > 32)
                {
                    err = ERROR_BAD_OPERAND;
                    stop = TRUE;
                }
                else
                {
                    r1.i = r2.i >> r1.i;
                    push(r1)
                }
            }
            break;
            case OP_AND:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("AND: %d & %d", r2.i, r1.i);
                r1.i = r2.i & r1.i;
                push(r1)
            }
            break;
            case OP_OR:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("OR: %d | %d", r2.i, r1.i);
                r1.i = r2.i | r1.i;
                push(r1)
            }
            break;
            case OP_NOT:
            {
                pop(r1)
                DBGPRINT("NOT: ~%d", r1.i);
                r1.i = ~r1.i;
                push(r1)
            }
            break;
            case OP_XOR:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("XOR: %d ^ %d", r2.i, r1.i);
                r1.i = r2.i ^ r1.i;
                push(r1)
            }
            break;
            case OP_GT:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("GT: %d > %d", r2.i, r1.i);
                r1.b = r2.i > r1.i;
                push(r1)
            }
            break;
            case OP_LT:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("LT: %d < %d", r2.i, r1.i);
                r1.b = r2.i < r1.i;
                push(r1)
            }
            break;
            case OP_GTE:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("GTE: %d >= %d", r2.i, r1.i);
                r1.b = r2.i >= r1.i;
                push(r1)
            }
            break;
            case OP_LTE:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("LTE: %d <= %d", r2.i, r1.i);
                r1.b = r2.i <= r1.i;
                push(r1)
            }
            break;
            case OP_INTEQ:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("INTEQ: %d == %d", r2.i, r1.i);
                r1.b = r2.i == r1.i;
                push(r1)
            }
            break;
            case OP_STREQ:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("STREQ: %s == %s", r2.s, r1.s);
                r1.b = strcmp(r2.s, r1.s) == 0;
                push(r1)
            }
            break;
            case OP_BOOLEQ:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("BOOLEQ: %s == %s", r2.b ? "true" : "false", r1.b ? "true" : "false");
                r1.b = r2.b == r1.b;
                push(r1)
            }
            break;
            case OP_INTNE:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("INTNE: %d != %d", r2.i, r1.i);
                r1.b = r2.i != r1.b;
                push(r1)
            }
            break;
            case OP_STRNE:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("STRNE: %s != %s", r2.s, r1.s);
                r1.b = strcmp(r2.s, r1.s) != 0;
                push(r1)
            }
            break;
            case OP_BOOLNE:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("BOOLNE: %s != %s", r2.b ? "true" : "false", r1.b ? "true" : "false");
                r1.b = r2.b != r1.b;
                push(r1)
            }
            break;
            case OP_ANDL:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("ANDL: %s && %s", r2.b ? "true" : "false", r1.b ? "true" : "false");
                r1.b = r2.b && r1.b;
                push(r1)
            }
            break;
            case OP_ORL:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("ORL: %s || %s", r2.b ? "true" : "false", r1.b ? "true" : "false");
                r1.b = r2.b || r1.b;
                push(r1)
            }
            break;
            case OP_NOTL:
            {
                pop(r1)
                DBGPRINT("NOTL: ! %s", r1.b ? "true" : "false");
                r1.b = !r1.b;
                push(r1)
            }
            break;
            case OP_NEG:
            {
                pop(r1)
                DBGPRINT("NEG: -%d", r1.i);
                r1.i = -r1.i;
                push(r1)
            }
            break;
            case OP_JFALSE:
            {
                // load value into reg without changing stack
                pop(r1)
                push(r1)
                arg(offset)
                DBGPRINT("JFALSE: if %s == false -> %d", r1.i ? "true" : "false", offset);
                if (!r1.b)
                    vm->ip = vm->code + offset; // absolute
            }
            break;
            case OP_JTRUE:
            {
                pop(r1)
                push(r1)
                arg(offset)
                DBGPRINT("JTRUE: if %s == true -> %d", r1.i ? "true" : "false", offset);
                if (r1.b)
                    vm->ip = vm->code + offset;
            }
            break;
            case OP_JMP:
            {
                arg(offset)
                DBGPRINT("JMP: -> %d", offset);
                vm->ip = vm->code + offset;
            }
            break;
            case OP_PUSHINT:
            {
                arg(val)
                DBGPRINT("PUSHINT: %d", val);
                r1.i = val;
                push(r1)
            }
            break;
            case OP_PUSHBOOL:
            {
                arg(val)
                DBGPRINT("PUSHBOOL: %s", val ? "true" : "false");
                r1.b = val;
                push(r1)
            }
            break;
            case OP_PUSHSTRING:
            {
                arg(offset)
                r1.s = ac_arena_get_string(vm->data, offset);
                DBGPRINT("PUSHSTRING: %d (%s)", offset, r1.s);
                assert(r1.s != NULL);
                push(r1)
            }
            break;
            case OP_PUSHMODULE:
            {
                arg(offset)
                r2.s = ac_arena_get_string(vm->data, offset);
                assert(r2.s != NULL);
                DBGPRINT("PUSHMODULE: %d (%s)", offset, r2.s);
                ac_context_object *module = ac_context_get(vm->cpl->ctx, r2.s);
                if (module == NULL)
                {
                    err = ERROR_MODULE;
                    stop = TRUE;
                    break;
                }
                r1.o = module;
                push(r1)
            }
            break;
            case OP_OBJECT:
            {
                pop(r1)
                DBGPRINT("OBJECT: %p", r1.o);
                ac_context_object_get_data(r1.o, &r1);
                push(r1)
            }
            break;
            case OP_CONTAINS:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("CONTAINS: %s contains %s", r2.s, r1.s);
                r1.b = strstr(r2.s, r1.s) != NULL;
                push(r1)
            }
            break;
            case OP_ICONTAINS:
            {
                pop(r1)
                pop(r2)
                r1.s = lowercase(r1.s);
                r2.s = lowercase(r2.s);
                DBGPRINT("ICONTAINS: %s icontains %s", r2.s, r1.s);
                uint32_t b = strstr(r2.s, r1.s) != NULL;
                ac_free(r1.s);
                ac_free(r2.s);
                r1.b = b;
                push(r1)
            }
            break;
            case OP_STARTSWITH:
            {
                pop(r1)
                pop(r2)
                DBGPRINT("STARTSWITH: %s startswith %s", r2.s, r1.s);
                char *o = strstr(r2.s, r1.s);
                r1.b = o != NULL && o == r2.s; // ensure substring is at the beginning of r2.s
                push(r1);
            }
            break;
            case OP_ISTARTSWITH:
            {
                pop(r1)
                pop(r2)
                r1.s = lowercase(r1.s);
                r2.s = lowercase(r2.s);
                DBGPRINT("ISTARTWITH: %s istartswith %s", r2.s, r1.s);
                char *o = strstr(r2.s, r1.s);
                uint32_t b = o != NULL && o == r2.s;
                ac_free(r1.s);
                ac_free(r2.s);
                r1.b = b;
                push(r1)
            }
            break;
            case OP_ENDSWITH:
            {
                pop(r1)
                pop(r2)
                uint32_t r2l = strlen(r2.s);
                uint32_t r1l = strlen(r1.s);
                DBGPRINT("ENDSWITH: %s endswith %s", r2.s, r1.s);
                r1.b = r2l >= r1l && strncmp(r2.s + (r2l - r1l), r1.s, r2l) == 0;
                push(r1)
            }
            break;
            case OP_IENDSWITH:
            {
                pop(r1)
                pop(r2)
                r1.s = lowercase(r1.s);
                r2.s = lowercase(r2.s);
                DBGPRINT("IENDSWITH: %s iendswith %s", r2.s, r1.s);
                uint32_t r2l = strlen(r2.s);
                uint32_t r1l = strlen(r1.s);
                uint32_t b = r2l >= r1l && strncmp(r2.s + (r2l - r1l), r1.s, r2l) == 0;
                ac_free(r1.s);
                ac_free(r2.s);
                r1.b = b;
                push(r1)
            }
            break;
            case OP_IEQUALS:
            {
                pop(r1)
                pop(r2)
                r1.s = lowercase(r1.s);
                r2.s = lowercase(r2.s);
                DBGPRINT("IEQUALS: %s iequals %s", r2.s, r1.s);
                uint32_t b = strcmp(r2.s, r1.s) == 0;
                ac_free(r1.s);
                ac_free(r2.s);
                r1.b = b;
                push(r1)
            }
            break;
            case OP_MATCHES:
            {
                pop(r1) // flags
                pop(r2) // regex string
                uint32_t flags = r1.i;
                pop(r1) // string to match
                DBGPRINT("MATCHES: %s matches %s (flags = 0x%.8xu)", r1.s, r2.s, flags);
                int error_number;
                PCRE2_SIZE error_offset;
                pcre2_code *re = pcre2_compile((PCRE2_SPTR8)r2.s,
                    PCRE2_ZERO_TERMINATED,
                    flags,
                    &error_number,
                    &error_offset,
                    NULL);
                if (re == NULL)
                {
                    DBGPRINT("MATCH: error: pcre2_compile returned NULL");
                    r1.b = FALSE;
                    push(r1)
                    break;
                }
                pcre2_match_data *md = pcre2_match_data_create_from_pattern(re, NULL);
                int rc = pcre2_match(re, (PCRE2_SPTR8)r1.s, strlen(r1.s), 0, 0, md, NULL);
                pcre2_match_data_free(md);
                pcre2_code_free(re);
                r1.b = rc >= 0;
                push(r1)
            }
            break;
            case OP_STRLEN:
            {
                pop(r1)
                DBGPRINT("STRLEN: %s", r1.s);
                r1.i = strlen(r1.s);
                push(r1)
            }
            break;
            case OP_RULE:
            {
                arg(offset)
                r1.s = ac_arena_get_string(vm->data, offset);
                DBGPRINT("RULE: call rule '%s'", r1.s);
                assert(r1.s != NULL);
                r2.i = __ac_compiler_find_rule_idx_by_name(vm->cpl, r1.s);
                assert(r2.i != -1);
                assert(r2.i < vm->cpl->nrules);
                ac_rule_entry e = vm->cpl->rule_table[r2.i];
                r2.b = FALSE;
                unsigned char *ret = vm->ip;
                err = ac_vm_exec_code(vm, vm->code + e.code_offset, &r2.b);
                vm->ip = ret;
                if (err != ERROR_SUCCESS)
                {
                    stop = TRUE;
                    break;
                }
                r1.b = r2.b;
                push(r1)
            }
            break;
            case OP_CALL:
            {
                pop(r1)
                ac_object *args = ac_alloc(r1.i * sizeof(ac_object));
                for (uint32_t i = 0; i < r1.i; i++)
                    pop(args[i]);
                ac_module_function fn = ac_context_object_get_function(r1.o);
                DBGPRINT("CALL: %p (argc = %d)", fn, r1.i);
                assert(fn != NULL);
                ac_object tmp = {0};
                err = fn(r1.o, args, &tmp);
                ac_free(args);
                if (err != ERROR_SUCCESS)
                {
                    stop = TRUE;
                    break;
                }
                r1 = tmp;
                push(r1)
            }
            break;
            case OP_FIELD:
            {
                pop(r1)
                arg(offset)
                r2.s = ac_arena_get_string(vm->data, offset);
                DBGPRINT("FIELD: %p.%s", r1.o, r2.s);
                assert(r2.s != NULL);
                r1.o = ac_context_object_get(r1.o, r2.s);
                assert(r1.o != NULL);
                push(r1)
            }
            break;
            case OP_INDEX:
            {
                pop(r1) // idx
                pop(r2) // object
                DBGPRINT("INDEX: %p[%d]", r2.o, r1.i);
                r2.b = ac_context_object_get_array_item(r2.o, r1.i, &r1);
                if (!r2.b)
                {
                    err = ERROR_INDEX;
                    stop = TRUE;
                    break;
                }
                push(r1)
            }
            break;
            case OP_LOAD:
            {
                arg(accum)
                DBGPRINT("LOAD: <-accum %d", accum);
                assert(accum < VM_ACCUM_MAX);
                push(regs[accum]);
            }
            break;
            case OP_STORE:
            {
                pop(r1)
                arg(accum)
                DBGPRINT("STORE: ->accum %d", accum);
                assert(accum < VM_ACCUM_MAX);
                regs[accum] = r1;
            }
            break;
            case OP_POP:
            {
                pop(r1)
            }
            break;
            default:
            {
                err = ERROR_OPERATION;
                stop = TRUE;
            }
        }
    }
    if (err != ERROR_SUCCESS)
        *result = FALSE;
    return err;
}
