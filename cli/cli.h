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
