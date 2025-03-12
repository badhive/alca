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

#ifndef AC_MODULE_H
#define AC_MODULE_H

#include <alca/utils.h>
#include <alca/context.h>

typedef ac_context_object ac_module;

typedef int (*ac_module_event_unmarshaller)(ac_module *module, const unsigned char *edata);

typedef void (*ac_module_freer)(ac_module *module);

/** Adds a global enum value to the specified module - accessible by 'module_name.value'
 *
 * @param module module object, returned by ac_module_create
 * @param value the C enum. This should be the enum name, NOT the resolved value. (e.g. 'FILE_CREATE', not, '0x1')
 */
#define ac_module_add_enum(module, value) \
    ac_module_add_field( \
    module, \
    #value, \
    AC_FIELD_TYPE_INTEGER); \
    ac_module_set_uint32_field(module, #value, value)

/** Create a module object.
 *
 * @param name name of module
 * @param version module version
 * @param free module freeing function
 * @return module object
 */
AC_API ac_module *ac_module_create(const char *name, uint32_t version, ac_module_freer free);

/** Create a new field within a module or field object.
 *
 * @param parent parent object
 * @param field_name name of field
 * @param field_type field type (e.g. integer, string, array, object)
 * @return field object
 */
AC_API ac_module *ac_module_add_field(ac_module *parent, const char *field_name, ac_field_type field_type);

/** Retrieves a module field object from a parent object.
 *
 * @param parent parent object
 * @param field_name name of field
 * @return field object
 */
AC_API ac_module *ac_module_get_field(ac_module *parent, const char *field_name);

/** Set the function that unmarshals event data received from this module's corresponding sensor.
 * This function is responsible
 *
 * @param module top-level module object (from ac_module_create)
 * @param unmarshal unmarshal function that receives event data
 */
AC_API void ac_module_set_unmarshaller(ac_module *module, ac_module_event_unmarshaller unmarshal);

/** Appends an object to a module field of type array.
 *
 * @param field field object
 * @param field_type type of item (same as array element type)
 * @param data array item value
 * @return TRUE if append was successful and FALSE otherwise
 */
AC_API int ac_module_array_field_append(ac_module *field, int field_type, ac_object *data);

/** (Call from unmarshal) assigns an integer value to a module field.
 *
 * @param parent parent object
 * @param field_name name of field
 * @param value integer value
 */
AC_API void ac_module_set_uint32_field(ac_module *parent, const char *field_name, uint32_t value);

/** (Call from unmarshal) assigns a string value to a module field via string copy.
 *
 * @param parent parent object
 * @param field_name name of field
 * @param value string value
 */
AC_API void ac_module_set_string_field(ac_module *parent, const char *field_name, char *value);

/** (Call from unmarshal) assigns a boolean value to a module field.
 *
 * @param parent parent object
 * @param field_name name of field
 * @param value boolean value
 */
AC_API void ac_module_set_bool_field(ac_module *parent, const char *field_name, int value);

/** Sets the C function for a top-level module field.
 *
 * @param field field object
 * @param fn C function
 * @param args function argument types
 * (b = bool, i = int, s = string) - e.g. "bsis" = function accepts bool, string, int, string
 */
AC_API void ac_module_set_function(ac_module *field, ac_module_function fn, const char *args);

/** Creates an empty struct object for an array of struct fields (AC_FIELD_TYPE_ARRAY | AC_FIELD_TYPE_STRUCT).
 * This function is used to create a struct that will hold event data being processed in an unmarshal callback.
 *
 * @param array array field object
 * @return the created struct if successful and NULL otherwise
 */
AC_API ac_module *ac_module_create_item_for_struct_array(ac_module *array);

/** Gets the top-level module object returned by ac_module_create
 *
 * @param module field / child object
 * @return the top-level module object
 */
AC_API ac_module *ac_module_get_toplevel_module(ac_module *module);

/** Gets module-specific data that was stored in the module object
 *
 * @param module top-level module object
 * @return module-specific stored data
 */
AC_API void *ac_module_get_context(ac_module *module);

/** Store module-specific data in the module object
 *
 * @param module top-level module object
 * @param data module data
 */
AC_API void ac_module_set_context(ac_module *module, void *data);

#endif //AC_MODULE_H
