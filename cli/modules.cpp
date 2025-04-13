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

#include <filesystem>
#include <alca.h>

#include "alca.fb.hpp"
#include "modules.hpp"

using namespace alca::modules;
using namespace Sensor;

int file::unmarshal_callback(ac_module *module, const unsigned char *event_data, size_t size)
{
    // TODO solve world corruption
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

int process::unmarshal_callback(ac_module *module, const unsigned char *event_data, size_t size)
{
    const auto *event = flatbuffers::GetRoot<ProcessEvent>(event_data);
    ac_module_set_uint32_field(module, "action", event->action());
    ac_module_set_uint32_field(module, "pid", event->pid());
    ac_module_set_uint32_field(module, "self_pid", event->self_pid());
    utils::set_string_field(module, "name", event->name());
    utils::set_string_field(module, "command_line", event->command_line());

    utils::set_string_field(module, "image_name", event->image_name());
    return TRUE;
}

int network::unmarshal_callback(ac_module *module, const unsigned char *event_data, size_t size)
{
    const auto *event = flatbuffers::GetRoot<NetworkEvent>(event_data);
    ac_module_set_uint32_field(module, "action", event->action());

    ac_module_set_uint32_field(module, "tcp", event->tcp());
    ac_module_set_uint32_field(module, "udp", event->udp());
    ac_module_set_uint32_field(module, "ipv6", event->ipv6());

    utils::set_string_field(module, "local_addr", event->local_addr());
    ac_module_set_uint32_field(module, "local_port", event->local_port());
    utils::set_string_field(module, "remote_addr", event->remote_addr());
    ac_module_set_uint32_field(module, "remote_port", event->remote_port());
    return TRUE;
}

int registry::unmarshal_callback(ac_module *module, const unsigned char *event_data, size_t size)
{
    const auto *event = flatbuffers::GetRoot<RegistryEvent>(event_data);
    ac_module_set_uint32_field(module, "action", event->action());

    utils::set_string_field(module, "key_path", event->key_path());
    utils::set_string_field(module, "value_name", event->value_name());
    return TRUE;
}
