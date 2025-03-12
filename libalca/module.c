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
#include <alca/module.h>

ac_module *ac_module_add_field(ac_module *module, const char *field_name, ac_field_type field_type)
{
    return ac_context_object_add_field(module, field_name, field_type);
}

ac_module *ac_module_create(const char *name, uint32_t version, ac_module_freer free)
{
    ac_context_object *ctxo = ac_context_create_module_object(name, version);
    if (ctxo)
        ac_context_object_set_freer(ctxo, free);
    return ctxo;
}

void ac_module_set_unmarshaller(ac_module *module, ac_module_event_unmarshaller unmarshal)
{
    return ac_context_object_set_unmarshaller(module, unmarshal);
}

void module_set_field_data_by_name(ac_module *parent, const char *field_name, ac_object *data)
{
    ac_module *field = ac_module_get_field(parent, field_name);
    if (field)
        ac_context_object_set_data(field, data);
}

void ac_module_set_uint32_field(ac_module *field, const char *field_name, uint32_t value)
{
    module_set_field_data_by_name(field, field_name, &(ac_object){ .i = value });
}

void ac_module_set_string_field(ac_module *field, const char *field_name, char *value)
{
    char *s = ac_alloc(strlen(value) + 1);
    strcpy(s, value);
    ac_object o = {0};
    o.s = s;
    module_set_field_data_by_name(field, field_name, &o);
}

void ac_module_set_bool_field(ac_module *field, const char *field_name, int value)
{
    module_set_field_data_by_name(field, field_name, &(ac_object){ .b = (uint32_t)value });
}

ac_module *ac_module_get_field(ac_module *parent, const char *field_name)
{
    return ac_context_object_get(parent, field_name);
}

void ac_module_set_function(ac_module *field, ac_module_function fn, const char *args)
{
    ac_context_object_set_function(field, fn, args);
}

int ac_module_array_field_append(ac_module *field, int field_type, ac_object *data)
{
    if (field_type == AC_FIELD_TYPE_STRING)
    {
        char *s = ac_alloc(strlen(data->s) + 1);
        strcpy(s, data->s);
        data->s = s;
    }
    int ok = ac_context_object_append_array_item(field, data);
    if (!ok && field_type == AC_FIELD_TYPE_STRING)
        ac_free(data->s);
    return ok;
}

ac_module *ac_module_create_item_for_struct_array(ac_module *array)
{
    return ac_context_object_create_struct_for_array(array);
}

ac_module *ac_module_get_toplevel_module(ac_module *module)
{
    return ac_context_object_get_module(module);
}

void *ac_module_get_context(ac_module *module)
{
    return ac_context_object_get_module_extended(module);
}

void ac_module_set_context(ac_module *module, void *data)
{
    return ac_context_object_set_module_extended(module, data);
}
