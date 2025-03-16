## Modules

A module is an object used by ALCA to unmarshal / deserialize event data received from a sensor. One module is
responsible for precisely one event type. However, a sensor can provide events of multiple types, as long as there
is a module corresponding to each event type.

Every module is named, and requires 3 functions with the following signatures:

1. A module loader. This is called when ALCA VM's context is initialised. The function name must be in the following format.
    ```c
    ac_module * <module_name>_load_callback();
    ```
2. A module unloader. This is called at the end of execution. The function name must be in the following format.
    ```c
    void <module_name>_unload_callback(ac_module *module);
    ```
3. An event unserialiser / unmarshaller. The function can have any name, but must have the same return type and
   parameters as below, as well as be assigned to the `ac_module` object in the module load callback.
    ```c
   void <module_name>_event_unmarshaller(ac_module *module, const unsigned char *event_data);
    ```

### Loader

A module loader initialises the module object, including the fields, their types, and any C functions that the module
exposes to the engine. There are 6 types: integers, strings, booleans, structs, arrays (which can hold 1 of
the previous 4 types), and functions. Here's an example module loader that was developed for testing.
This module is used for most of the rule examples shown in the docs.

```c
ac_module *ac_test_module_file_callback()
{
    enum file_event_action
    {
        FILE_CREATE,
        FILE_DELETE,
        FILE_MODIFY,
        FILE_RENAME,
    };
    ac_module *module = ac_module_create("file", AC_VERSION(0, 0, 0), NULL);
    ac_module_set_unmarshaller(module, ac_test_module_file_unmarshal);
    ac_module_add_field(module, "action", AC_FIELD_TYPE_INTEGER);
    ac_module_add_field(module, "size", AC_FIELD_TYPE_INTEGER);
    ac_module_add_field(module, "extension", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "path", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "name", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "num_sections", AC_FIELD_TYPE_INTEGER);

    ac_module *section_list = ac_module_add_field(module, "sections", AC_FIELD_TYPE_ARRAY | AC_FIELD_TYPE_STRUCT);
    ac_module_add_field(section_list, "name", AC_FIELD_TYPE_STRING);
    ac_module_add_field(section_list, "size", AC_FIELD_TYPE_INTEGER);
    ac_module_add_enum(module, FILE_CREATE);
    ac_module_add_enum(module, FILE_DELETE);
    ac_module_add_enum(module, FILE_MODIFY);
    ac_module_add_enum(module, FILE_RENAME);
    return module;
}
```

A module object is created with `ac_module_create`. Fields are added with `ac_module_add_field`, which returns a
sub-object that can be used to add further sub-fields to struct / struct array fields. `ac_module_set_unmarshaller`
assigns an unmarshalling function to the module, which will set each field value based on received event data.

### Unloader

A module unloader is a simple function that frees any extra data associated with the module. This would typically be
the module context, set with `ac_module_set_context`, to store any extra information that can be accessed at unmarshal
time or by module functions. If no data needs to be freed, the unloader can just return immediately.

### Unmarshaller

A module unmarshaller receives the top-level module object and event data, and is responsible for interpreting the data
and assigning values to each field in the module object. The flexibility of this approach means that sensors can send and
modules can receive events in whatever format they want. You may choose to use protobuf, or just JSON. The following test
unmarshaller uses a custom binary format.

```c
// The test data will be received by the unmarshaller in this format

    unsigned char fakedata[] =
        "\x00\x00\x00\x00" // file.action = FILE_CREATE
        "\xFF\x00\x00\x00" // size = 255
        "exe\0" // extension = exe
        "\\Windows\\Temp\\mal.exe\0" // path
        "mal.exe\0" // name
        "\x03\x00\x00\x00" // num sections

        ".text\0" // section #1 name
        "\x0F\x00\x00\x00" // section #1 size

        ".bss\0" // section #2 name
        "\xD0\x00\x00\x00" // section #2 size

        ".rdata\0" // section #3 name
        "\x20\x00\x00\x00"; // section #3 size
```

As shown below, it uses its own API to parse this data and assigns it to corresponding fields using the module API.

```c
int ac_test_module_file_unmarshal(ac_module *module, const unsigned char *edata)
{
    test_unmarshal_t um = {.index = 0, .data = edata};
    ac_module *sections = ac_module_get_field(module, "sections");
    ac_module_set_uint32_field(module, "action", test_unmarshal_uint32(&um));
    ac_module_set_uint32_field(module, "size", test_unmarshal_uint32(&um));
    ac_module_set_string_field(module, "extension", test_unmarshal_string(&um));
    ac_module_set_string_field(module, "path", test_unmarshal_string(&um));
    ac_module_set_string_field(module, "name", test_unmarshal_string(&um));
    uint32_t len = test_unmarshal_uint32(&um);
    ac_module_set_uint32_field(module, "num_sections", len);
    for (int i = 0; i < len; i++)
    {
        char *name = test_unmarshal_string(&um);
        uint32_t size = test_unmarshal_uint32(&um);
        ac_module *obj = ac_module_create_item_for_struct_array(sections);
        ac_module_set_string_field(obj, "name", name);
        ac_module_set_uint32_field(obj, "size", size);
        ac_object o = {.o = obj};
        ac_module_array_field_append(sections, AC_FIELD_TYPE_STRUCT, &o);
    }
    return TRUE;
}
```

This is less than 30 lines of code! This is practically all that is required of a module.

### Integrating your module

Once your module code has been written (ideally in a single source file), you need to recompile ALCA to have it
recognised by the engine.

1. Add an entry for your module to the module.list file, located in `alca/libalca/include/alca/`.
   ```
   MODULE(<module_name>)
   ```
2. Add your source file(s) to `ALCA_SOURCES` in CMakeLists.txt.
   ```cmake
   set(ALCA_SOURCES libalca/lexer.c
        libalca/utils.c
        libalca/parser.c
        libalca/expr.c,
        ...
        path/to/your/module.c)
   ```
3. Build!
   ```shell
   cmake -B build . && cmake --build build/ --target alca
   ```

More information about writing sensors and modules will be available at a later stage, as ALCA's flagship sensor is
currently in active development.