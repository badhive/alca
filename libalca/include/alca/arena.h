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

#ifndef AC_ARENA_H
#define AC_ARENA_H

#include <stdint.h>

typedef struct ac_arena ac_arena;

// Arena is used for storing string literals in a contiguous block of memory.
// An arena is created by the compiler for the data section of the malware.
// When a string is stored in the arena, it returns the string's offset to the
// start address of the memory block. This offset is used for string operations
// in the VM (all strings are passed by reference).

ac_arena *ac_arena_create(uint32_t initial_size);

void ac_arena_destroy(ac_arena *arena);

uint32_t ac_arena_add_string(ac_arena *arena, char *str, uint32_t length);

char *ac_arena_get_string(ac_arena *arena, uint32_t offset);

uint32_t ac_arena_find_string(ac_arena *arena, char *str);

uint32_t ac_arena_add_bytes(ac_arena *arena, void *data, uint32_t length);

void ac_arena_replace_bytes(ac_arena *arena, uint32_t offset, void *data, uint32_t length);

uint32_t ac_arena_add_uint32(ac_arena *arena, uint32_t value);

void ac_arena_prepend_bytes(ac_arena *arena, void *data, uint32_t length);

uint32_t ac_arena_add_code(ac_arena *arena, char op);

uint32_t ac_arena_add_code_with_arg(ac_arena *arena, char op, uint32_t arg);

uint32_t ac_arena_size(ac_arena *arena);

void *ac_arena_data(ac_arena *arena);

#endif //AC_ARENA_H
