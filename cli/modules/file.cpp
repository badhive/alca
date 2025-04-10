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
#include "flatbuffers/idl.h"

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

    utils::set_string_field(module, "owner", event->owner());
    ac_module_set_uint32_field(module, "size", event->size());
    ac_module_set_uint32_field(module, "mode", event->mode());
    ac_module_set_uint32_field(module, "created_at", event->created_at());
    ac_module_set_uint32_field(module, "modified_at", event->modified_at());
    utils::set_string_field(module, "new_name", event->new_name());

    ac_module *pe_field = ac_module_get_field(module, "pe");
    const WinPE *pe = event->pe();
    if (pe == nullptr)
        return TRUE;

    utils::set_string_field(pe_field, "arch", pe->arch());
    ac_module_set_uint32_field(pe_field, "is_dotnet", pe->is_dotnet());

    ac_module *sections_field = ac_module_get_field(pe_field, "sections");
    ac_module *imports_field = ac_module_get_field(pe_field, "imports");
    const auto *sections = pe->sections();
    const auto *imports = pe->imports();

    if (sections == nullptr)
        return TRUE;
    if (imports == nullptr)
        return TRUE;

    ac_module_set_uint32_field(pe_field, "section_count", sections->size());
    ac_module_set_uint32_field(pe_field, "import_count", imports->size());

    for (int i = 0; i < sections->size(); ++i)
    {
        const FileSection *section = sections->Get(i);
        ac_module *object = ac_module_create_item_for_struct_array(sections_field);
        utils::set_string_field(object, "name", section->name());
        ac_module_set_uint32_field(object, "size", section->size());
        ac_module_set_uint32_field(object, "entropy", section->entropy());
        ac_object o = {.o = object };
        ac_module_array_field_append(sections_field, AC_FIELD_TYPE_STRUCT, &o);
    }

    for (int i = 0; i < imports->size(); ++i)
    {
        const FileImport *import = imports->Get(i);
        const auto *functions = import->functions();

        ac_module *object = ac_module_create_item_for_struct_array(imports_field);
        ac_module *functions_object = ac_module_get_field(object, "functions");

        utils::set_string_field(object, "name", import->name());
        for (size_t j = 0; j < functions->size(); j++)
        {
            ac_object fn_name = { .s = const_cast<char *>(functions->Get(j)->c_str()) };
            ac_module_array_field_append(functions_object, AC_FIELD_TYPE_STRING, &fn_name);
        }
        ac_object o = {.o = object };
        ac_module_array_field_append(imports_field, AC_FIELD_TYPE_STRUCT, &o);
    }
    return TRUE;
}
