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

#include <alca.h>
#include <flatbuffers/flatbuffers.h>

#define ALCA_MODULE(module) \
    namespace module \
    { \
        int unmarshal_callback(ac_module *module, const unsigned char *event_data, size_t data_size); \
        \
        static ac_module_table_entry module_entry = { \
            #module, \
            ac_default_##module##_load_callback, \
            ac_default_##module##_unload_callback, \
            unmarshal_callback, \
        }; \
    } \

namespace alca::modules
{
    namespace utils
    {
        void set_string_field(ac_module *module, const char *field_name, const flatbuffers::String* value);
    }

    ALCA_MODULE(file)

    ALCA_MODULE(process)

    ALCA_MODULE(network)

    ALCA_MODULE(registry)

    inline void update_defaults(ac_compiler *compiler)
    {
        ac_compiler_include_module(compiler, &file::module_entry);
        ac_compiler_include_module(compiler, &process::module_entry);
        ac_compiler_include_module(compiler, &network::module_entry);
        ac_compiler_include_module(compiler, &registry::module_entry);
    }
}

#undef ALCA_MODULE

#endif //MODULES_H
