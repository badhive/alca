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


#include <alca.h>

#include "cli.h"

int main(int argc, char *argv[])
{
    cli_t *cli;
    CLI_BOOL help = 0;
    CLI_STRING binary = "";
    CLI_STRING ruleFile = "";
    CLI_STRING ruleDir = "";
    CLI_INT localPort = 4164;
    CLI_STRING remoteHost = "";
    cli_create("alca", "event-based rule engine for dynamic analysis and research", &cli);

    cli_add_arg(
        cli,
        CLI_ARG_BOOL | CLI_ARG_HELP,
        "--help",
        "-h",
        "show this help message and exit",
        0,
        (void**)&help);

    cli_add_arg(cli,
        CLI_ARG_STRING | CLI_ARG_POSITIONAL,
        "binary",
        "",
        "binary to analyse",
        0,
        (void **)&binary);

    cli_add_arg(cli,
        CLI_ARG_STRING,
        "--rule-file",
        "-f",
        "alca rule file (.alca)",
        0,
        (void **)&ruleFile);

    cli_add_arg(cli,
        CLI_ARG_STRING,
        "--rule-folder",
        "-d",
        "folder containing alca rules",
        0,
        (void **)&ruleDir);

    cli_add_arg(cli,
        CLI_ARG_INT,
        "--local",
        "-l",
        "(local mode) connect to sensor through specified port",
        0,
        (void **)&localPort);

    cli_add_arg(cli,
        CLI_ARG_STRING,
        "--remote-host",
        "-r",
        "(remote mode) connect to remote sensor at specified address",
        0,
        (void **)&remoteHost);

    if (cli_parse_args(cli, argc, argv) < 0)
        return -1;

    if (help)
    {
        cli_print_help(cli);
        return -1;
    }

    cli_destroy(cli);
    return 0;
}
