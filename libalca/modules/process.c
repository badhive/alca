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

ac_module *ac_default_process_load_callback()
{
    enum process_event_action
    {
        ProcessStart = 0,
        ProcessEnd = 1,
        CreateRemoteThread = 2,
        CreateLocalThread = 3,
        TerminateLocalThread = 4,
        TerminateRemoteThread = 5,
        ImageLoad = 6,
        ImageUnload = 7,
        AllocLocal = 8,
        AllocRemote = 9,
    };
    ac_module *module = ac_module_create("process", ALCA_VERSION, NULL);

    ac_module_add_field(module, "action", AC_FIELD_TYPE_INTEGER);

    ac_module_add_field(module, "pid", AC_FIELD_TYPE_INTEGER);
    ac_module_add_field(module, "name", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "self_pid", AC_FIELD_TYPE_INTEGER);
    ac_module_add_field(module, "command_line", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "image_name", AC_FIELD_TYPE_STRING);

    ac_module_add_enum(module, ProcessStart);
    ac_module_add_enum(module, ProcessEnd);
    ac_module_add_enum(module, CreateRemoteThread);
    ac_module_add_enum(module, CreateLocalThread);
    ac_module_add_enum(module, TerminateLocalThread);
    ac_module_add_enum(module, TerminateRemoteThread);
    ac_module_add_enum(module, ImageLoad);
    ac_module_add_enum(module, ImageUnload);
    ac_module_add_enum(module, AllocLocal);
    ac_module_add_enum(module, AllocRemote);

    return module;
}

// developer-implemented
void ac_default_process_unload_callback(const ac_module *module) {}

// developer-implemented
int ac_default_process_unmarshal_callback(ac_module *module, const unsigned char *event_data, size_t size)
{
    return FALSE;
}
