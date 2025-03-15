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

#define ALCA_VERSION AC_VERSION( 0, 0, 1 )
#define ALCA_MAGIC 0x41434c4

#define b2l(a) ac_u32be_to_u32le(a)
#define l2b(a) ac_u32le_to_u32be(a)

#include <stdint.h>
#include <alca/errors.h>

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

uint32_t ac_u32le_to_u32be(uint32_t x);

ac_error ac_read_file(const char *filename, char **buffer, uint32_t *size);

char *__ac_path_join(int, ...);

#endif //AC_UTILS_H
