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
#include <alca/errors.h>

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

uint32_t ac_u32le_to_u32be(uint32_t x)
{
    if (!LITTLE_ENDIAN)
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

ac_error ac_read_file(const char *filename, char **buffer, uint32_t *size)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
        return ERROR_COMPILER_FILE;
    fseek(file, 0, SEEK_END);
    size_t fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *fbuf = ac_alloc(fsize+1);

    fbuf[fsize] = '\0';
    fread(fbuf, 1, fsize, file);
    fclose(file);
    if (buffer && size)
    {
        *buffer = fbuf;
        *size = fsize;
    }
    else
        free(fbuf);
    return ERROR_SUCCESS;
}
