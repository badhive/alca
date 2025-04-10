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

#include "modules.hpp"
#include "alca.fb.hpp"

using namespace alca::modules;
using namespace Sensor;

int file::unmarshal_callback(ac_module *module, const unsigned char *event_data)
{
    const auto *event = flatbuffers::GetRoot<FileEvent>(event_data);
    ac_module_set_uint32_field(module, "action", event->action());
    utils::set_string_field(module, "name", event->name());
    utils::set_string_field(module, "path", event->path());
    utils::set_string_field(module, "directory", event->directory());
    utils::set_string_field(module, "extension", event->extension());
    utils::set_string_field(module, "new_name", event->new_name());
    ac_module_set_uint32_field(module, "is_directory", event->is_directory());

    return TRUE;
}
