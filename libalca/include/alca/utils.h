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

#ifndef AC_UTILS_H
#define AC_UTILS_H

#ifdef _WIN32
#ifdef ALCA_BUILD_DLL
#define AC_API __declspec(dllexport)
#elif ALCA_BUILD_STATIC
#define AC_API
#else
#define AC_API __declspec(dllimport)
#endif
#else
#define AC_API
#endif

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif
#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

#define AC_MAX_PATH_COUNT 32
#define lengthof(x) (long)(sizeof(x)/sizeof(x[0]))

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN *(char*)(&(int){1})
#endif

#define AC_VERSION( maj, min, pch ) \
((maj << 24) & 0xFF000000) | ((min << 16) & 0x00FF0000) | (pch & 0x0000FFFF)

#define b2l(a) ac_u32be_to_u32le(a)

#include <stdint.h>

/** Joins any number path elements into a single path. The resulting path can be freed with ac_free.
 * Does not take into account / resolve relative elements.
 *
 * @param ... variable list of path elements
 * @return pointer to new path.
 */
#define AC_PATH_JOIN( ... ) __ac_path_join( 0, __VA_ARGS__, NULL )

void ac_free(void *ptr);

void *ac_alloc(unsigned int size);

void *ac_realloc(void *ptr, unsigned int size);

char *ac_str_extend(char *str, char c);

uint32_t ac_u32be_to_u32le(uint32_t x);

char *__ac_path_join(int, ...);

#endif //AC_UTILS_H
