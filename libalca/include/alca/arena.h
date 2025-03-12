/*
Copyright (c) 2025, pygrum. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
