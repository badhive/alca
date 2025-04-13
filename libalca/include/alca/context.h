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

#ifndef AC_CONTEXT_H
#define AC_CONTEXT_H

#include <alca/types.h>

typedef struct ac_context ac_context;
typedef struct ac_context_object ac_context_object;
typedef struct ac_object ac_object;
typedef struct ac_context_env_item ac_context_env_item;

// vm stack is array of ac_objects
struct ac_object
{
    int type;
    union
    {
        uint32_t i; // int
        uint32_t b; // bool
        char *s; // string
        ac_context_object *o; // object
        struct
        {
            uint32_t len;
            ac_object *array;
        } a; // array
    };
};

struct ac_context_env_item
{
    char *name;
    ac_stmt_type type;
    ac_token_type tok_type;
    char *src;
    void *ext;
};

typedef void (*ac_context_object_freer)(ac_context_object *object);

typedef ac_context_object ac_module;

typedef ac_module *(*ac_module_load_callback)();

typedef void (*ac_module_unload_callback)(const ac_module *);

typedef int (*ac_module_event_unmarshaller)(ac_module *module, const unsigned char *edata, size_t edata_len);

typedef ac_error (*ac_module_function)(ac_module *fn_object, ac_object *args, ac_object *result);

typedef struct ac_module_table_entry
{
    const char *name;
    ac_module_load_callback load_callback;
    ac_module_unload_callback unload_callback;
    ac_module_event_unmarshaller unmarshal_callback;
} ac_module_table_entry;

ac_context *ac_context_new();

#define AC_FIELD_TYPE_TOPLEVEL 128
#define AC_FIELD_TYPE_STRING 2
#define AC_FIELD_TYPE_INTEGER 4
#define AC_FIELD_TYPE_BOOLEAN 8
#define AC_FIELD_TYPE_STRUCT 16
#define AC_FIELD_TYPE_FUNCTION 32

// flag used for constant globals within a module.
#define AC_FIELD_TYPE_ARRAY 0x10000000
#define AC_FIELD_TYPE_CONSTANT 0x80000000


void ac_context_free(ac_context *ctx);

/** [INTERNAL] Creates a new global object used by the compiler to store and access custom event data.
 * The name of the object must be the same as the "event type" name.
 * This must NOT be used for module objects.
 *
 * @param ctx context object
 * @param name name of global object
 * @return global object instance, or NULL if an object with the same name exists
 */
ac_context_object *ac_context_create_object(ac_context *ctx, const char *name);

/** [INTERNAL] Like ac_context_create_object, except the object is not bound to a context. This is used
 * for modules so that they can be loaded once imported by a rule file.
 *
 * @param name name of module
 * @param version module version
 * @return created object instance
 */
ac_context_object *ac_context_create_module_object(const char *name, uint32_t version);

/** [INTERNAL] Creates a new field or sub-field for a global object. This field's offset is used for a quick lookup at runtime.
 * Only a top-level object or an object with AC_FIELD_TYPE_OBJECT property can use this function.
 *
 * @param object object or object field
 * @param name name of the field
 * @param type field data type
 * @return the newly created field, or null
 */
ac_context_object *ac_context_object_add_field(
    ac_context_object *object,
    const char *name,
    ac_field_type type);

/** [INTERNAL] Set data for a field in an object. This is typically called in the event unmarshalling callback.
 * Ensure that the corresponding union field for the object's type is set.
 *
 * @param object object or object field
 * @param value ac_object union
 */
void ac_context_object_set_data(ac_context_object *object, ac_object *value);

/** [INTERNAL] internal use only
 *
 * @param object context object
 * @param value received value
 */
void ac_context_object_get_data(ac_context_object *object, ac_object *value);

/** [INTERNAL] Turns a context object field into a function, binding it to a C function.
 *
 * @param object object or object field
 * @param c_function associated C function
 * @param args string of arg types, e.g. "sib" (s=string, i=int, b=boolean)
 */
void ac_context_object_set_function(ac_context_object *object, ac_module_function c_function, const char *args);

void ac_context_object_set_freer(ac_context_object *object, ac_context_object_freer callback);

// [internal]
ac_module_function ac_context_object_get_function(ac_context_object *object);

/** [INTERNAL] define a stdlib function.
 *
 * @param ctx context object
 * @param name name of function
 * @param c_function associated C function
 * @param args string of arg types, e.g. "sib" (s=string, i=int, b=boolean)
 * @param return_type return type of the function
 * @return object if successful, NULL if not (or if object with matching name already exists)
 */
ac_context_object *ac_context_add_toplevel_function(ac_context *ctx, const char *name, ac_module_function c_function,
                                                    const char *args, ac_field_type return_type);

/** [INTERNAL] internal use only
 *
 * @param fn function object
 * @param args string of arg types
 * @param count expected arg count
 * @param types expected types
 * @return boolean
 */
ac_error ac_context_validate_function_call(ac_context_object *fn, const char *args, unsigned int *count, const char **types);

/** [INTERNAL] Get information about an object.
 *
 * @param object object or object field
 * @param name receive name of object
 * @param type receive type of object
 */
void ac_context_object_info(ac_context_object *object, char **name, ac_field_type *type);

/** [INTERNAL] Retrieves a field or sub-field from an object.
 *
 * @param object object or object field
 * @param name name of sub-field
 * @return sub-field if it exists, or null
 */
ac_context_object *ac_context_object_get(ac_context_object *object, const char *name);

/** [INTERNAL] Retrieves a top-level object (e.g. a module) from the global context.
 *
 * @param ctx context
 * @param name name of module / object
 * @return object if it exists, or null
 */
ac_context_object *ac_context_get(ac_context *ctx, const char *name);

void *ac_context_get_environment(ac_context *ctx);

/** [INTERNAL] Retrieves module callback data from the global context.
 *
 * @param ctx context
 * @param name name of module
 * @param module receives module entry if it exists
 * @return TRUE if module exists, FALSE otherwise
 */
int ac_context_get_module(ac_context *ctx, const char *name, ac_module_table_entry *module);

uint32_t ac_context_object_get_module_version(ac_context_object *object);

size_t ac_context_object_get_field_count(ac_context_object *object);

/** [INTERNAL] Adds a module is triggered when a module is imported.
 * Once the module is imported, the context object is freed internally.
 *
 * @param ctx context object
 * @param module module table entry.
 */
void ac_context_add_module(ac_context *ctx, ac_module_table_entry *module);

void ac_context_load_modules(ac_context *ctx);

int ac_context_object_unmarshal_evtdata(ac_context_object *object, unsigned char *edata, size_t edata_size);

ac_context_object *ac_context_object_get_module(ac_context_object *object);

void ac_context_object_clear_module_data(ac_context_object *module);
// [internal]
int ac_context_object_get_array_item(ac_context_object *object, uint32_t index, ac_object *value);

int ac_context_object_append_array_item(ac_context_object *object, ac_object *value);

ac_context_object *ac_context_object_create_struct_for_array(ac_context_object *array);

void *ac_context_object_get_module_extended(ac_context_object *module);

void ac_context_object_set_module_extended(ac_context_object *module, void *data);

#endif //AC_CONTEXT_H
