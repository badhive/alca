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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"

struct ac_cli_arg
{
    const char *name;
    const char *shorthand;
    const char *description;
    uint32_t type;
    int required;
    void *value;
    void *default_value;
    int parsed;
};

struct cli_t
{
    const char *name;
    const char *desc;
    int nargs;
    int npos;
    int longname;
    int longdesc;
    int longdefault;

    int parsed;
    struct ac_cli_arg args[CLI_MAX_ARGS];
};

int cli_create(const char *app, const char *description, cli_t **pcli)
{
    assert(pcli != NULL);
    cli_t *cli = calloc(1, sizeof(cli_t));
    cli->name = app;
    cli->desc = description;
    cli->longname = 4;
    cli->longdesc = 11;
    cli->longdefault = 7;
    *pcli = cli;
    return 0;
}

void cli_destroy(cli_t *cli)
{
    if (cli)
        free(cli);
}

int cli_add_arg(
    cli_t *cli,
    const uint32_t type,
    const char *name,
    const char *shorthand,
    const char *description,
    const int required,
    void **value)
{
    assert(shorthand != NULL);
    assert(description != NULL);
    assert(value != NULL);
    if (!(type & CLI_ARG_POSITIONAL))
        assert(name != NULL && strlen(name) > 2 && name[0] == '-' && name[1] == '-');
    int lname, ldesc, ldefault = 0;
    for (int i = 0; i < cli->nargs; i++)
    {
        if (strcmp(cli->args[i].name, name) == 0)
            return -1;
    }
    unsigned int lsh = strlen(shorthand);
    if (lsh)
    {
        assert(lsh == 2 && shorthand[0] == '-');
    }
    struct ac_cli_arg *arg = &cli->args[cli->nargs++];
    arg->name = name;
    arg->shorthand = shorthand;
    arg->description = description;
    arg->type = type;
    arg->required = required;

    char ibuf[sizeof(int) * 8 + 1];
    arg->value = value;
    arg->default_value = *value;
    if (type & CLI_ARG_BOOL)
        ldefault = 4;
    else if (type & CLI_ARG_INT)
        ldefault = (int) strlen(itoa(*(int *) value, ibuf, 10));
    else if (type & CLI_ARG_STRING)
        ldefault = (int) strlen(*value);

    lname = (int) strlen(arg->name);
    ldesc = (int) strlen(arg->description);
    if (lname > cli->longname)
        cli->longname = lname; // to include `--`
    if (ldesc > cli->longdesc)
        cli->longdesc = ldesc;
    if (ldefault > cli->longdefault)
        cli->longdefault = ldefault;
    if (type & CLI_ARG_POSITIONAL)
        cli->npos++;
    return 0;
}

int cli_parse_value(struct ac_cli_arg *arg, void *value)
{
    char *endptr;
    uint32_t type = arg->type & ~CLI_ARG_POSITIONAL & ~CLI_ARG_HELP;
    switch (type)
    {
        case CLI_ARG_BOOL: *(CLI_BOOL *) arg->value = 1;
            break;
        case CLI_ARG_INT: *(CLI_INT *) arg->value = strtol(value, &endptr, 10);
            break;
        case CLI_ARG_STRING: arg->value = value;
            break;
        default: ;
    }
    if (type & CLI_ARG_INT)
    {
        if (*endptr != '\0')
        {
            fprintf(stderr, "error: %s's value is not an integer\n", arg->name);
            return -1;
        }
    }
    arg->parsed = 1;
    return 0;
}

int cli_parse_args(cli_t *cli, int argc, char *argv[])
{
    int npos = 0;
    int gothelp = 0;
    int wasflag = 0;
    for (int i = 1; i < argc; i++)
    {
        struct ac_cli_arg *cli_arg = NULL;
        const char *arg = argv[i];
        if (arg[0] == '-')
        {
            if (arg[1] == '-')
            {
                for (int j = 0; j < cli->nargs; j++)
                {
                    if (strcmp(cli->args[j].name, arg) == 0 &&
                        !(cli->args[j].type & CLI_ARG_POSITIONAL))
                    {
                        cli_arg = &cli->args[j];
                        break;
                    }
                }
            } else
            {
                for (int j = 0; j < cli->nargs; j++)
                {
                    if (strcmp(cli->args[j].shorthand, arg) == 0 &&
                        !(cli->args[j].type & CLI_ARG_POSITIONAL))
                    {
                        cli_arg = &cli->args[j];
                        break;
                    }
                }
            }
            if (!cli_arg)
            {
                fprintf(stderr, "error: unrecognised flag '%s'\n", arg);
                return -1;
            }
            if (cli_arg->type & CLI_ARG_HELP)
                gothelp = 1;
            if (cli_arg->parsed)
            {
                fprintf(stderr, "error: flag '%s' already specified\n", arg);
                return -1;
            }
            if (!(cli_arg->type & CLI_ARG_BOOL) && (i + 1 >= argc || argv[i + 1][0] == '-'))
            {
                fprintf(stderr, "error: no value specified for flag '%s'\n", arg);
                return -1;
            }
            void *value = NULL;
            if (!(cli_arg->type & CLI_ARG_BOOL))
                value = argv[++i];
            else
            {
                value = &(CLI_BOOL){1};
                wasflag = 1;
            }
            if (cli_parse_value(cli_arg, value) < 0)
                return -1;
        } else
        {
            for (int j = 0; j < cli->nargs; j++)
            {
                if (cli->args[j].type & CLI_ARG_POSITIONAL && !cli->args[j].parsed)
                {
                    if (cli_parse_value(&cli->args[j], (void *) arg) < 0)
                        return -1;
                }
            }
            if (!wasflag)
                npos++;
            else
                wasflag = 0;
        }
    }
    if (npos != cli->npos && !gothelp)
    {
        fprintf(stderr, "error: expected %d positional argument(s), got %d\n", cli->npos, npos);
        return -1;
    }
    for (int i = 0; i < cli->nargs; i++)
    {
        struct ac_cli_arg *cli_arg = &cli->args[i];
        if (!cli_arg->parsed && cli_arg->required)
        {
            fprintf(stderr, "error: required flag '%s' was not provided\n", cli_arg->name);
            return -1;
        }
    }
    cli->parsed = 1;
    return 0;
}

int cli_print_help(cli_t *cli)
{
    int padding = 2;
    printf("\n%s: %s\n\n", cli->name, cli->desc);
    printf("usage: %s ", cli->name);
    for (int i = 0; i < cli->nargs; i++)
    {
        struct ac_cli_arg *cli_arg = &cli->args[i];
        if (cli_arg->type & CLI_ARG_POSITIONAL)
            printf("<%s> ", cli_arg->name);
    }
    printf("%s\n\n", cli->nargs - cli->npos ? "[flags]" : "");
    int printpos = 1;
    char *hdr = "args:\n";
    char *fmt = "%*s       %-*s\n";
PRINT:
    printf(hdr);
    if (printpos)
    {
        printf(fmt, cli->longname + padding, "name", cli->longdesc + padding, "description");
        printf(fmt, cli->longname + padding, "+--+", cli->longdesc + padding, "+---------+");
    } else
    {
        printf(fmt,
               cli->longname + padding, "name",
               " ",
               2 + padding, "",
               cli->longdesc + padding, "description",
               cli->longdefault + padding, "default");
        printf(fmt,
               cli->longname + padding, "+--+",
               " ",
               2 + padding, "",
               cli->longdesc + padding, "+---------+",
               cli->longdefault + padding, "+-----+");
    }
    for (int i = 0; i < cli->nargs; i++)
    {
        char *str = "";
        char ibuf[sizeof(int) * 8 + 1];
        long long int doprint = 0;
        struct ac_cli_arg *arg = &cli->args[i];
        switch (arg->type & ~CLI_ARG_POSITIONAL & ~CLI_ARG_HELP)
        {
            case CLI_ARG_BOOL: str = (size_t) arg->default_value ? "true" : "false";
                break;
            case CLI_ARG_INT: str = itoa((int) (size_t) arg->default_value, ibuf, 10);
                break;
            case CLI_ARG_STRING: str = arg->default_value;
                break;
            default: ;
        }
        if (printpos)
        {
            doprint = arg->type & CLI_ARG_POSITIONAL;
            if (doprint)
                printf(fmt, cli->longname + padding, arg->name, cli->longdesc + padding, arg->description);
        } else
        {
            doprint = !(arg->type & CLI_ARG_POSITIONAL);
            if (doprint)
            {
                printf(fmt,
                       cli->longname + padding, arg->name,
                       strlen(arg->shorthand) > 0 ? "," : " ",
                       2 + padding, arg->shorthand,
                       cli->longdesc + padding, arg->description,
                       cli->longdefault + padding, str);
            }
        }
    }
    if (printpos)
    {
        printpos = 0;
        fmt = "%*s%s %-*s %-*s %-*s\n";
        hdr = "\nflags:\n";
        goto PRINT;
    }
    printf("\n");
    return 0;
}
