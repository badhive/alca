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

#ifndef AC_DEFAULTS_H
#define AC_DEFAULTS_H

#include <alca/module.h>

#define DEF_MODULE(name) \
    ac_module *ac_default_##name##_load_callback(); \
    void ac_default_##name##_unload_callback(const ac_module *module); \
    int ac_default_##name##_unmarshal_callback(ac_module *module, const unsigned char *event_data, size_t size); \

DEF_MODULE(file)
DEF_MODULE(process)
DEF_MODULE(network)
DEF_MODULE(registry)

#undef DEF_MODULE

#endif //AC_DEFAULTS_H
