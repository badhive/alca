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

ac_module *ac_default_registry_load_callback()
{
    enum registry_event_action
    {
        RegOpenKey = 0,
        RegCreateKey = 1,
        RegSetValue = 2,
        RegDeleteKey = 3,
        RegDeleteValue = 4
    };
    ac_module *module = ac_module_create("registry", ALCA_VERSION, NULL);

    ac_module_add_field(module, "action", AC_FIELD_TYPE_INTEGER);

    ac_module_add_field(module, "key_path", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "value_name", AC_FIELD_TYPE_STRING);

    ac_module_add_enum(module, RegOpenKey);
    ac_module_add_enum(module, RegCreateKey);
    ac_module_add_enum(module, RegSetValue);
    ac_module_add_enum(module, RegDeleteKey);
    ac_module_add_enum(module, RegDeleteValue);

    return module;
}

// developer-implemented
void ac_default_registry_unload_callback(const ac_module *module) {}

// developer-implemented
int ac_default_registry_unmarshal_callback(ac_module *module, const unsigned char *event_data, size_t size)
{
    return FALSE;
}
