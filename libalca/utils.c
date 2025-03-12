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

#if defined(_WIN32)
#define PATH_SEPARATOR '\\'
#else
    #define PATH_SEPARATOR '/'
#endif

#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <alca/utils.h>

/** Frees memory allocated by ac_ functions. Should only be used with functions that explicitly allow it.
 *
 * @param ptr pointer to dynamically allocated memory
 */
void ac_free(void *ptr)
{
    free(ptr);
}

/** Allocates dynamic memory of a specified size. The resulting memory can be freed with ac_free.
 *
 * @param size size of memory block in bytes
 * @return memory block
 */
void *ac_alloc(unsigned int size)
{
    void *ptr = calloc(size, sizeof(char));
    if (ptr == NULL)
    {
        printf("allocation failed (could not allocate %d bytes)\n", size);
        exit(-1);
    }
    return ptr;
}

/** Reallocates a memory block with a new size and panics on failure. The resulting memory can be freed with ac_free.
 *
 * @param ptr memory block to resize
 * @param size new size of memory block in bytes
 * @return resized memory block
 */
void *ac_realloc(void *ptr, unsigned int size)
{
    ptr = realloc(ptr, size);
    if (ptr == NULL)
    {
        printf("allocation failed (could not re-allocate %d bytes)\n", size);
        exit(-1);
    }
    return ptr;
}

/** Extends a string by one byte. The resulting string can be freed with ac_free.
 *
 * @param str string to extend. Cannot be a string literal, must be a dynamically allocated string.
 * If NULL, a null-terminated string containing the single character is returned.
 * @param c character to add to the string
 * @return extended string, which can be freed with ac_free.
 */
char *ac_str_extend(char *str, char c)
{
    size_t len = 1;
    char *newStr = NULL;
    if (!str)
        newStr = ac_alloc(len + 1);
    else
    {
        len = strlen(str) + 1;
        newStr = ac_realloc(str, len + 1);
    }
    newStr[len - 1] = c;
    newStr[len] = '\0';
    return newStr;
}

uint32_t ac_u32be_to_u32le(uint32_t x)
{
    if (LITTLE_ENDIAN)
        return x;
    return (x >> 24) & 0x000000FF |
           (x >> 8) & 0x0000FF00 |
           (x << 8) & 0x00FF0000 |
           (x << 24) & 0xFF000000;
}

char *__ac_path_join(int n, ...)
{
    int count = 0;
    char *parts[AC_MAX_PATH_COUNT];
    unsigned len = 0;
    unsigned idx = 0;
    va_list args;
    va_start(args, n);
    while (1)
    {
        if (count >= AC_MAX_PATH_COUNT)
            break;
        char *part = va_arg(args, char*);
        if (!part)
            break;
        len += strlen(part) + 1;
        parts[count] = part;
        count++;
    }
    char *newPath = malloc(len);
    newPath[len - 1] = '\0';
    for (int i = 0; i < count; i++)
    {
        strcpy(newPath + idx, parts[i]);
        idx += strlen(parts[i]);
        // don't add separator on final path element
        if (i < count - 1)
        {
            newPath[idx] = PATH_SEPARATOR;
            idx++;
        }
    }
    return newPath;
}
