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
        FileCreate = 0,
        FileRename = 1,
        FileDelete = 2,
        FileModify = 3,
    };
    ac_module *module = ac_module_create("file", ALCA_VERSION, NULL);

    ac_module_add_field(module, "action", AC_FIELD_TYPE_INTEGER);

    ac_module_add_field(module, "name", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "path", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "directory", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "extension", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "new_name", AC_FIELD_TYPE_STRING);

    ac_module_add_enum(module, FileCreate);
    ac_module_add_enum(module, FileRename);
    ac_module_add_enum(module, FileDelete);
    ac_module_add_enum(module, FileModify);

    return module;
}

// developer-implemented
void ac_default_file_unload_callback(const ac_module *module) {}

// developer-implemented
int ac_default_file_unmarshal_callback(ac_module *module, const unsigned char *event_data, size_t size)
{
    return FALSE;
}
