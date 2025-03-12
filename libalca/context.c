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

#include <stdio.h>
#include <string.h>

#include <alca/errors.h>
#include <alca/utils.h>
#include "hashmap.h"

#include <alca/context.h>

typedef struct ac_module_loader
{
    const char *name;
    ac_module_load_callback load_callback;
} module_loader;

struct ac_context
{
    struct hashmap *objects;
    struct hashmap *module_callbacks;
    struct hashmap *environment;
};

struct ac_context_object
{
    const char *name;
    uint32_t version;
    ac_context_object *parent;
    struct hashmap *fields;
    ac_field_type field_type; // return value type if field is a function
    uint32_t offset;
    ac_object object;

    // if type == function
    ac_module_function c_function;
    uint32_t arg_count;
    const char *arg_types;

    // if top-level module, use function to unmarshal event data into context object
    ac_context_object_event_unmarshaller unmarshal;
    ac_context_object_freer free;
    void *extended_data;
};

int context_object_cmp(const void *a, const void *b, void *c)
{
    const ac_context_object *obja = a;
    const ac_context_object *objb = b;
    return strcmp(obja->name, objb->name);
}

uint64_t context_object_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const ac_context_object *obj = item;
    return hashmap_murmur(obj->name, strlen(obj->name), seed0, seed1);
}

int module_cmp(const void *a, const void *b, void *c)
{
    const module_loader *obja = a;
    const module_loader *objb = b;
    return strcmp(obja->name, objb->name);
}

uint64_t module_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const module_loader *obj = item;
    return hashmap_murmur(obj->name, strlen(obj->name), seed0, seed1);
}

int env_item_cmp(const void *a, const void *b, void *c)
{
    const ac_context_env_item *obja = a;
    const ac_context_env_item *objb = b;
    return strcmp(obja->name, objb->name);
}

uint64_t env_item_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const ac_context_env_item *obj = item;
    return hashmap_murmur(obj->name, strlen(obj->name), seed0, seed1);
}

void ac_context_object_free_internals(void *object)
{
    ac_context_object *o = object;
    hashmap_free(o->fields);
    if (o->free) o->free(o);
}

ac_context *ac_context_new()
{
    ac_context *ctx = ac_alloc(sizeof(ac_context));
    ctx->objects = hashmap_new(sizeof(ac_context_object), 0, 0, 0,
                               context_object_hash, context_object_cmp, ac_context_object_free_internals, NULL);
    ctx->module_callbacks = hashmap_new(sizeof(module_loader), 0, 0, 0,
                                        module_hash, module_cmp, NULL, NULL);
    ctx->environment = hashmap_new(sizeof(ac_context_env_item), 0, 0, 0,
                               env_item_hash, env_item_cmp, NULL, NULL);
    return ctx;
}

void ac_context_free(ac_context *ctx)
{
    hashmap_free(ctx->objects);
    hashmap_free(ctx->module_callbacks);
    hashmap_free(ctx->environment);
    ac_free(ctx);
}

ac_context_object *ac_context_create_object(ac_context *ctx, const char *name)
{
    if (hashmap_get(ctx->objects, &(ac_context_object){.name = name}))
        return NULL;
    ac_context_object *obj = ac_context_create_module_object(name, 0);
    hashmap_set(ctx->objects, obj);
    ac_free(obj);
    return obj;
}

ac_context_object *ac_context_create_module_object(const char *name, uint32_t version)
{
    ac_context_object *obj = ac_alloc(sizeof(ac_context_object));
    obj->name = name;
    obj->version = version;
    obj->field_type = AC_FIELD_TYPE_TOPLEVEL | AC_FIELD_TYPE_STRUCT;
    obj->fields = hashmap_new(sizeof(ac_context_object), 0, 0, 0,
                              context_object_hash, context_object_cmp, ac_context_object_free_internals, NULL);
    return obj;
}

ac_context_object *ac_context_add_toplevel_function(ac_context *ctx, const char *name, ac_module_function c_function,
                                                    const char *args, ac_field_type return_type)
{
    if (hashmap_get(ctx->objects, &(ac_context_object){.name = name}))
        return NULL;
    ac_context_object *obj = ac_alloc(sizeof(ac_context_object));
    obj->name = name;
    obj->field_type = AC_FIELD_TYPE_TOPLEVEL | AC_FIELD_TYPE_FUNCTION | return_type;
    obj->c_function = c_function;
    obj->arg_types = args;
    obj->arg_count = strlen(args);
    hashmap_set(ctx->objects, obj);
    return obj;
}

ac_error ac_context_validate_function_call(ac_context_object *fn, const char *args, unsigned int *count, const char **types)
{
    if (!(fn->field_type & AC_FIELD_TYPE_FUNCTION))
        return ERROR_BAD_LITERAL;
    *count = fn->arg_count;
    *types = fn->arg_types;
    if (args == NULL)
    {
        if (fn->arg_count == 0)
            return TRUE;
        return ERROR_BAD_CALL;
    }
    if (strlen(args) != fn->arg_count)
        return ERROR_BAD_CALL;
    if (strncmp(fn->arg_types, args, fn->arg_count + 1) != 0)
        return ERROR_UNEXPECTED_TYPE;
    return ERROR_SUCCESS;
}

void ac_context_object_set_function(ac_context_object *object, ac_module_function c_function, const char *args)
{
    if (!(object->field_type & AC_FIELD_TYPE_FUNCTION && object->field_type & AC_FIELD_TYPE_TOPLEVEL)) return;
    object->c_function = c_function;
    object->arg_types = args;
    object->arg_count = strlen(args);
}

void ac_context_object_set_freer(ac_context_object *object, ac_context_object_freer callback)
{
    object->free = callback;
}

ac_module_function ac_context_object_get_function(ac_context_object *object)
{
    if (!(object->field_type & AC_FIELD_TYPE_FUNCTION)) return NULL;
    return object->c_function;
}

/* Most fields are assumed to have an entry in the data header.
 * Object fields are never accessed directly by value, so they are not present in the header, but the fields are.
 * Object field offsets are calculated at compile time.
 *
 * STRING: 4-byte entry is the offset to the first char of the string (null-terminated)
 * INTEGER: 4-byte entry is the 32-bit integer itself
 * BOOLEAN: 4-byte entry is the bool value (0x00000001 or 0x00000000)
 *
 * After the headers, the actual data begins.
 */
ac_context_object *ac_context_object_add_field(
    ac_context_object *object,
    const char *name,
    ac_field_type type)
{
    if (hashmap_get(object->fields, &(ac_context_object){.name = name}))
        return NULL;
    if (!(object->field_type & AC_FIELD_TYPE_STRUCT))
        return NULL;
    ac_context_object *field = ac_alloc(sizeof(ac_context_object));
    field->name = name;
    field->parent = object;
    field->field_type = type;
    field->fields = hashmap_new(sizeof(ac_context_object), 0, 0, 0,
                                context_object_hash, context_object_cmp, ac_context_object_free_internals, NULL);

    hashmap_set(object->fields, field);
    ac_free(field);
    return (ac_context_object *) hashmap_get(object->fields, &(ac_context_object){.name = name});
}

void ac_context_object_set_data(ac_context_object *object, ac_object *value)
{
    if (value)
        memcpy(&object->object, value, sizeof(ac_object));
}

void ac_context_object_get_data(ac_context_object *object, ac_object *value)
{
    if (value)
        memcpy(value, &object->object, sizeof(ac_object));
}

void ac_context_object_info(ac_context_object *object, char **name, ac_field_type *type)
{
    if (name)
        *name = (char*)object->name;
    if (type)
        *type = object->field_type;
}

ac_context_object *ac_context_object_get(ac_context_object *object, const char *name)
{
    return (ac_context_object *) hashmap_get(object->fields, &(ac_context_object){.name = name});
}

size_t ac_context_object_get_field_count(ac_context_object *object)
{
    if (object->fields)
        return hashmap_count(object->fields);
    return 0;
}

ac_context_object *ac_context_get(ac_context *ctx, const char *name)
{
    return (ac_context_object *) hashmap_get(ctx->objects, &(ac_context_object){.name = name});
}

void *ac_context_get_environment(ac_context *ctx)
{
    return ctx->environment;
}

int ac_context_get_module(ac_context *ctx, const char *name, ac_module_load_callback *callback)
{
    module_loader *ldr = (module_loader *) hashmap_get(ctx->module_callbacks, &(module_loader){.name = name});
    if (!ldr)
        return FALSE;
    *callback = ldr->load_callback;
    return TRUE;
}

uint32_t ac_context_object_get_module_version(ac_context_object *object)
{
    return object->version;
}

void ac_context_add_module_load_callback(ac_context *ctx, const char *module_name, ac_module_load_callback callback)
{
    module_loader module = {.name = module_name, .load_callback = callback};
    hashmap_set(ctx->module_callbacks, &module);
}

int ac_context_load_module(ac_context *ctx, const char *module_name)
{
    const module_loader *module = hashmap_get(ctx->module_callbacks, &(module_loader){.name = module_name});
    if (!module)
        return 0;
    if (hashmap_get(ctx->objects, &(ac_context_object){.name = module_name}))
        return 1; // already loaded
    ac_context_object *object = module->load_callback(); // object was created with ac_context_create_module_object
    hashmap_set(ctx->objects, object);
    ac_free(object);
    return 1;
}

void ac_context_object_set_unmarshaller(ac_context_object *object, ac_context_object_event_unmarshaller unmarshal)
{
    if (!(object->field_type & AC_FIELD_TYPE_STRUCT && object->field_type & AC_FIELD_TYPE_TOPLEVEL)) // only work on modules
        return;
    object->unmarshal = unmarshal;
}

int ac_context_object_unmarshal_evtdata(ac_context_object *object, unsigned char *edata)
{
    if (!object->unmarshal)
        return FALSE;
    return object->unmarshal(object, edata);
}

ac_context_object *ac_context_object_get_module(ac_context_object *object)
{
    ac_context_object *upper = object;
    while (upper->parent)
        upper = object->parent;
    if (!(upper->field_type & AC_FIELD_TYPE_STRUCT | AC_FIELD_TYPE_TOPLEVEL))
        return NULL;
    return upper;
}

void *ac_context_object_get_module_extended(ac_context_object *module)
{
    if (!(module->field_type & AC_FIELD_TYPE_STRUCT | AC_FIELD_TYPE_TOPLEVEL))
        return NULL;
    return module->extended_data;
}

void ac_context_object_set_module_extended(ac_context_object *module, void *data)
{
    if (!(module->field_type & AC_FIELD_TYPE_STRUCT | AC_FIELD_TYPE_TOPLEVEL))
        return;
    module->extended_data = data;
}

void ac_context_object_clear_module_data(ac_context_object *module)
{
    void *f;
    size_t i = 0;
    while (hashmap_iter(module->fields, &i, &f))
    {
        ac_context_object *o = f;
        if (o->field_type == AC_FIELD_TYPE_STRING)
        {
            if (o->object.s) ac_free(o->object.s);
        } else if (o->field_type == AC_FIELD_TYPE_STRUCT)
        {
            if (o->object.o) ac_context_object_clear_module_data(o->object.o);
        } else if (o->field_type & AC_FIELD_TYPE_ARRAY)
        {
            for (int j = 0; j < o->object.a.len; j++)
            {
                if (o->field_type & AC_FIELD_TYPE_STRUCT)
                {
                    if (o->object.a.array[j].o) ac_context_object_clear_module_data(o->object.a.array[j].o);
                } else if (o->field_type & AC_FIELD_TYPE_ARRAY)
                {
                    if (o->object.s) ac_free(o->object.s);
                }
            }
            ac_free(o->object.a.array);
        }
        memset(&o->object, 0, sizeof(ac_object));
    }
}

int ac_context_object_get_array_item(ac_context_object *object, uint32_t index, ac_object *value)
{
    if (!(object->field_type & AC_FIELD_TYPE_ARRAY))
        return FALSE;
    if (index >= object->object.a.len)
        return FALSE;
    *value = object->object.a.array[index];
    return TRUE;
}

int ac_context_object_append_array_item(ac_context_object *object, ac_object *value)
{
    if (!(object->field_type & AC_FIELD_TYPE_ARRAY))
        return FALSE;
    if (!object->object.a.array)
        object->object.a.array = ac_alloc(sizeof(ac_object));
    else
        object->object.a.array = ac_realloc(object->object.a.array, (object->object.a.len + 1) * sizeof(ac_object));
    object->object.a.array[object->object.a.len++] = *value;
    return TRUE;
}

ac_context_object *ac_context_object_create_struct_for_array(ac_context_object *array)
{
    void *item;
    size_t i = 0;
    ac_context_object *object = ac_context_create_module_object("", 0);
    if (!(array->field_type & AC_FIELD_TYPE_ARRAY && array->field_type & AC_FIELD_TYPE_STRUCT))
        return NULL;
    while (hashmap_iter(array->fields, &i, &item))
    {
        const ac_context_object *o = item;
        ac_context_object_add_field(object, o->name, o->field_type);
    }
    return object;
}
