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

#include <stdio.h>
#include <string.h>

#include <alca/utils.h>
#include "hashmap.h"

#include <alca/arena.h>

#define AC_ARENA_START_SIZE 256
#define AC_ARENA_CHUNK_SIZE 64

struct ac_arena
{
    char *data;
    uint32_t size;
    uint32_t capacity;
    struct hashmap *strmap;
};

typedef struct
{
    char *str;
    uint32_t offset;
} arena_item;

int arena_item_cmp(const void *a, const void *b, void *c)
{
    const arena_item *obja = a;
    const arena_item *objb = b;
    return strcmp(obja->str, objb->str);
}

void arena_extend(ac_arena *arena, uint32_t size)
{
    if (arena->capacity < arena->size + size)
    {
        while (arena->capacity < arena->size + size)
            arena->capacity += AC_ARENA_CHUNK_SIZE;
        arena->data = ac_realloc(arena->data, arena->capacity);
    }
}

uint64_t arena_item_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const arena_item *obj = item;
    return hashmap_sip(obj->str, strlen(obj->str), seed0, seed1);
}

ac_arena *ac_arena_create(uint32_t initial_size)
{
    ac_arena *arena = ac_alloc(sizeof(ac_arena));
    if (initial_size)
        arena->capacity = initial_size;
    else
        arena->capacity = AC_ARENA_START_SIZE;
    arena->strmap = hashmap_new(sizeof(arena_item), 0, 0, 0,
                                arena_item_hash, arena_item_cmp, NULL, NULL);
    arena->data = ac_alloc(arena->capacity);
    return arena;
}

void ac_arena_destroy(ac_arena *arena)
{
    if (arena->data) ac_free(arena->data);
    hashmap_free(arena->strmap);
    arena->capacity = 0;
    arena->size = 0;
    ac_free(arena);
}

uint32_t ac_arena_add_string(ac_arena *arena, char *str, uint32_t length)
{
    uint32_t offset = arena->size;
    // prevents us from adding same strings multiple times
    const arena_item *item = hashmap_get(arena->strmap, &(arena_item){ .str = str });
    if (item)
        return item->offset;

    arena_extend(arena, length + 1);
    memcpy(arena->data + arena->size, str, length + 1);
    arena->size += length + 1;
    hashmap_set(arena->strmap, &(arena_item){ str, offset });
    return offset;
}

// used to get a string arg from module variadic args. called internally
char *ac_arena_get_string(ac_arena *arena, uint32_t offset)
{
    // make sure offset is valid and string exists in saved map
    if (hashmap_get(arena->strmap, &(arena_item){ .str = arena->data + offset }))
        return arena->data + offset;
    return NULL;
}

// primarily to check if string exists
uint32_t ac_arena_find_string(ac_arena *arena, char *str)
{
    const arena_item *item = hashmap_get(arena->strmap, &(arena_item){ .str = str });
    // make sure offset is valid and string exists in saved map
    if (item != NULL)
        return item->offset;
    return -1;
}

uint32_t ac_arena_add_bytes(ac_arena *arena, void *data, uint32_t length)
{
    uint32_t offset = arena->size;
    arena_extend(arena, length);
    memcpy(arena->data + arena->size, data, length);
    arena->size += length;
    return offset;
}

void ac_arena_replace_bytes(ac_arena *arena, uint32_t offset, void *data, uint32_t length)
{
    memcpy(arena->data + offset, data, length);
}

uint32_t ac_arena_add_uint32(ac_arena *arena, uint32_t value)
{
    uint32_t offset = arena->size;
    arena_extend(arena, sizeof(uint32_t));
    memcpy(arena->data + arena->size, &(uint32_t){b2l(value)}, sizeof(uint32_t));
    arena->size += sizeof(uint32_t);
    return offset;
}

void ac_arena_prepend_bytes(ac_arena *arena, void *data, uint32_t length)
{
    arena_extend(arena, length);
    memcpy(arena->data + length, arena->data, arena->size); // move forward
    memcpy(arena->data, data, length); // prepend
    arena->size += length;
}

uint32_t ac_arena_add_code(ac_arena *arena, char op)
{
    uint32_t offset = arena->size;
    arena_extend(arena, 1);
    arena->data[arena->size] = op;
    arena->size++;
    return offset;
}

uint32_t ac_arena_add_code_with_arg(ac_arena *arena, char op, uint32_t arg)
{
    uint32_t offset = ac_arena_add_code(arena, op);
    ac_arena_add_uint32(arena, arg);
    return offset;
}

uint32_t ac_arena_size(ac_arena *arena)
{
    return arena->size;
}

void *ac_arena_data(ac_arena *arena)
{
    return arena->data;
}
