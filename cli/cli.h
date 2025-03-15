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

#ifndef CLI_H
#define CLI_H

#include <stdint.h>

#define CLI_ARG_INT 1
#define CLI_ARG_BOOL 2
#define CLI_ARG_STRING 4

#define CLI_ARG_POSITIONAL 0x10
#define CLI_ARG_HELP 0x20

#define CLI_MAX_ARGS 20

typedef struct cli_t cli_t;
typedef size_t CLI_BOOL;
typedef long long int CLI_INT;
typedef char* CLI_STRING;

int cli_create(const char *app, const char *description, cli_t **pcli);

void cli_destroy(cli_t *cli);

int cli_add_arg(
    cli_t *cli,
    uint32_t type,
    const char *name,
    const char *shorthand,
    const char *description,
    int required,
    void **value
);

int cli_parse_args(cli_t *cli, int argc, char *argv[]);

int cli_print_help(cli_t *cli);

#endif //CLI_H
