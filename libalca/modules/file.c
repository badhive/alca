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

#include <alca.h>

#include "alca_reader.h"

// Convenient namespace macro to manage long namespace prefix.
#undef ns
// Specified in the schema.
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(Sensor, x)

// A helper to simplify creating vectors from C-arrays.
#define c_vec_len(V) (sizeof(V)/sizeof((V)[0]))

void file_unload_callback(ac_module *module) {}

int file_event_unmarshaller(ac_module *module, const unsigned char *event_data)
{
    ns(FileEvent_table_t) file_event = ns(FileEvent_as_root(event_data));

    ac_module_set_uint32_field(module, "action", ns(FileEvent_action(file_event)));

    ac_module_set_string_field(module, "name", (char *)ns(FileEvent_name(file_event)));
    ac_module_set_string_field(module, "path", (char *)ns(FileEvent_path(file_event)));
    ac_module_set_string_field(module, "directory", (char *)ns(FileEvent_directory(file_event)));
    ac_module_set_string_field(module, "extension", (char *)ns(FileEvent_extension(file_event)));

    ac_module_set_string_field(module, "owner", (char *)ns(FileEvent_owner(file_event)));
    ac_module_set_uint32_field(module, "size", ns(FileEvent_size(file_event)));
    ac_module_set_uint32_field(module, "mode", ns(FileEvent_mode(file_event)));
    ac_module_set_uint32_field(module, "created_at", ns(FileEvent_created_at(file_event)));
    ac_module_set_uint32_field(module, "modified_at", ns(FileEvent_modified_at(file_event)));

    ac_module_set_string_field(module, "new_name", (char *)ns(FileEvent_new_name(file_event)));

    ac_module *pe = ac_module_get_field(module, "pe");
    ns(WinPE_table_t) pe_table = ns(FileEvent_pe(file_event));

    ac_module_set_string_field(pe, "arch", (char *)ns(WinPE_arch(pe_table)));
    ac_module_set_uint32_field(pe, "is_dotnet", ns(WinPE_is_dotnet(pe_table)));

    ac_module *sections = ac_module_get_field(pe, "sections");
    ac_module *imports = ac_module_get_field(pe, "imports");
    ns(FileSection_vec_t) section_vec = ns(WinPE_sections(pe_table));
    ns(FileImport_vec_t) import_vec = ns(WinPE_imports(pe_table));

    size_t section_vec_len = ns(FileSection_vec_len(section_vec));
    size_t import_vec_len = ns(FileImport_vec_len(import_vec));
    ac_module_set_uint32_field(pe, "section_count", section_vec_len);
    ac_module_set_uint32_field(pe, "import_count", import_vec_len);

    for (size_t i = 0; i < section_vec_len; i++)
    {
        ns(FileSection_table_t) section_table = ns(FileSection_vec_at(section_vec, i));
        ac_module *object = ac_module_create_item_for_struct_array(sections);
        ac_module_set_string_field(object, "name", (char *)ns(FileSection_name(section_table)));
        ac_module_set_uint32_field(object, "size", ns(FileSection_size(section_table)));
        ac_module_set_uint32_field(object, "entropy", ns(FileSection_entropy(section_table)));
        ac_module_array_field_append(sections, AC_FIELD_TYPE_STRUCT, &(ac_object){.o = object});
    }

    for (size_t i = 0; i < import_vec_len; i++)
    {
        ns(FileImport_table_t) import_table = ns(FileImport_vec_at(section_vec, i));
        flatbuffers_string_vec_t functions = ns(FileImport_functions(import_table));
        size_t functions_count = flatbuffers_string_vec_len(functions);

        ac_module *object = ac_module_create_item_for_struct_array(imports);
        ac_module *functions_object = ac_module_get_field(object, "functions");

        ac_module_set_string_field(object, "name", (char *)ns(FileImport_name(import_table)));
        for (size_t j = 0; j < functions_count; j++)
        {
            char *str = (char *)flatbuffers_string_vec_at(functions, j);
            ac_module_array_field_append(functions_object, AC_FIELD_TYPE_STRING, &(ac_object){.s = str});
        }
        ac_module_array_field_append(imports, AC_FIELD_TYPE_STRUCT, &(ac_object){.o = object});
    }
    return TRUE;
}

ac_module *file_load_callback()
{
    enum file_event_action
    {
        FILE_CREATE,
        FILE_RENAME,
        FILE_DELETE,
        FILE_MODIFY,
    };
    ac_module *module = ac_module_create("file", ALCA_VERSION, NULL);
    ac_module_set_unmarshaller(module, file_event_unmarshaller);

    ac_module_add_field(module, "action", AC_FIELD_TYPE_INTEGER);

    ac_module_add_field(module, "name", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "path", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "directory", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "extension", AC_FIELD_TYPE_STRING);

    ac_module_add_field(module, "owner", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "size", AC_FIELD_TYPE_INTEGER);
    ac_module_add_field(module, "mode", AC_FIELD_TYPE_INTEGER);
    ac_module_add_field(module, "created_at", AC_FIELD_TYPE_INTEGER);
    ac_module_add_field(module, "modified_at", AC_FIELD_TYPE_INTEGER);

    ac_module *pe = ac_module_add_field(module, "pe", AC_FIELD_TYPE_STRUCT);
    ac_module_add_field(pe, "arch", AC_FIELD_TYPE_STRING);
    ac_module_add_field(pe, "is_dotnet", AC_FIELD_TYPE_INTEGER);
    ac_module_add_field(pe, "section_count", AC_FIELD_TYPE_INTEGER);
    ac_module_add_field(pe, "import_count", AC_FIELD_TYPE_INTEGER);

    ac_module *sections = ac_module_add_field(pe, "sections", AC_FIELD_TYPE_ARRAY | AC_FIELD_TYPE_STRUCT);
    ac_module_add_field(sections, "name", AC_FIELD_TYPE_STRING);
    ac_module_add_field(sections, "size", AC_FIELD_TYPE_INTEGER);
    ac_module_add_field(sections, "entropy", AC_FIELD_TYPE_INTEGER);

    ac_module *imports = ac_module_add_field(sections, "imports", AC_FIELD_TYPE_ARRAY | AC_FIELD_TYPE_STRUCT);
    ac_module_add_field(imports, "name", AC_FIELD_TYPE_STRING);
    ac_module_add_field(imports, "functions", AC_FIELD_TYPE_ARRAY | AC_FIELD_TYPE_STRING);

    ac_module_add_field(module, "new_name", AC_FIELD_TYPE_STRING);

    ac_module_add_enum(module, FILE_CREATE);
    ac_module_add_enum(module, FILE_RENAME);
    ac_module_add_enum(module, FILE_DELETE);
    ac_module_add_enum(module, FILE_MODIFY);

    return module;
}
