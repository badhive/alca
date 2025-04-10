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

#ifndef MODULES_H
#define MODULES_H

#include <flatbuffers/flatbuffers.h>
#include <alca.h>

namespace alca::modules
{
    namespace utils
    {
        void set_string_field(ac_module *module, const char *field_name, const flatbuffers::String* value);
    }

    namespace file
    {
        int unmarshal_callback(ac_module *module, const unsigned char *event_data);

        static ac_module_table_entry module_entry = {
            .name = "file",
            .load_callback = ac_default_file_load_callback,
            .unload_callback = ac_default_file_unload_callback,
            .unmarshal_callback = unmarshal_callback,
        };
    }

    inline void update_defaults(ac_compiler *compiler)
    {
        ac_compiler_include_module(compiler, &file::module_entry);
    }
}

#endif //MODULES_H
