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

#include <alca/defaults.h>

ac_module *ac_default_file_load_callback()
{
    enum file_event_action
    {
        FILE_CREATE = 0,
        FILE_RENAME = 1,
        FILE_DELETE = 2,
        FILE_MODIFY = 3,
    };
    ac_module *module = ac_module_create("file", ALCA_VERSION, NULL);

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

// developer-implemented
void ac_default_file_unload_callback(const ac_module *module) {}

// developer-implemented
int ac_default_file_unmarshal_callback(ac_module *module, const unsigned char *event_data)
{
    return FALSE;
}
