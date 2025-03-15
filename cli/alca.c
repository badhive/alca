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

#include <stdio.h>
#include <string.h>

#include <alca.h>
#include "cli.h"
#include "conn.h"

#define LOCAL_MODE 1
#define REMOTE_MODE 2

void flogf(FILE *fd, const char *format, ...) {
    time_t rawtime;
    struct tm *timeinfo;
    char time_str[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    fprintf(fd, "[%s] ", time_str);

    va_list args;
    va_start(args, format);
    vfprintf(fd, format, args);
    va_end(args);

    fprintf(fd, "\n");
}

int check_recv(SOCKET s, int rc)
{
    if (rc == 0)
    {
        flogf(stderr, "[error] connection to remote sensor has been closed");
        conn_close(s);
        return -1;
    }
    if (rc < 0)
    {
        flogf(stderr, "[error] failed to receive: %d", rc);
        conn_close(s);
        return -1;
    }
    return 0;
}

int await(SOCKET s, const char *binPath)
{
    // TODO
}

SOCKET submit_local(short port, const uint8_t *data, uint32_t size)
{
    ac_packet_handle hpacket;
    int rc = 0;
    uint32_t packet_size;
    ac_packet_create(AC_PACKET_LOCAL, AC_PACKET_DATA_LOCAL_SUBMIT, &hpacket);
    ac_packet_set_data(hpacket, data, size, 0);
    const uint8_t *pkdata = ac_packet_serialize(hpacket, &packet_size);
    SOCKET s = conn_connect("localhost", port);
    if (s == INVALID_SOCKET)
    {
        flogf(stderr, "[error] failed to connect to local port %d", port);
        goto end;
    }
    send(s, (const char *)&packet_size, sizeof(packet_size), 0);
    rc = send(s, (const char *)pkdata, packet_size, 0);
    if (rc == SOCKET_ERROR)
    {
        flogf(stderr, "[error] failed to submit binary to sensor");
        conn_close(s);
        s = INVALID_SOCKET;
    }
end:
    ac_packet_free_serialized((void *)pkdata);
    ac_packet_destroy(hpacket);
    return s;
}

#define BIN_CHUNK 1024
SOCKET submit_remote(const char *host, const char *binpath)
{
    ac_packet_handle hpacket;
    int rc = 0;
    char *end;
    FILE *file;
    size_t fsize;
    char *pport = strstr(host, ":");
    if (pport == NULL)
    {
        flogf(stderr, "[error] invalid remote host address provided");
        return INVALID_SOCKET;
    }
    int port = strtol(pport+1, &end, 10);
    if (port == 0 || port == LONG_MAX || port == LONG_MIN)
    {
        flogf(stderr, "[error] invalid remote port number");
        return INVALID_SOCKET;
    }
    char *hostname = ac_alloc(host - pport + 1);
    memcpy(hostname, host, host - pport + 1);
    SOCKET s = conn_connect(hostname, port);
    if (s == INVALID_SOCKET)
    {
        flogf(stderr, "[error] failed to connect to local port %d", port);
        ac_free(hostname);
        return s;
    }
    file = fopen(binpath, "rb");
    if (file == NULL)
    {
        flogf(stderr, "[error] failed to open file %s", binpath);
        return INVALID_SOCKET;
    }
    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    size_t bytesRead = 0;
    int sequence = 0;
    while (bytesRead < fsize)
    {
        char *buf = ac_alloc(BIN_CHUNK);
        if (fseek(file, (int)bytesRead, SEEK_SET) != 0)
        {
            flogf(stderr, "[error] failed to read file %s", binpath);
            ac_free(buf);
            conn_close(s);
            return INVALID_SOCKET;
        }
        size_t nread = fread(buf, 1, BIN_CHUNK, file);
        bytesRead += nread;
        if (bytesRead >= fsize)
            sequence = AC_PACKET_SEQUENCE_LAST;
        ac_packet_create(AC_PACKET_REMOTE, AC_PACKET_DATA_REMOTE_SUBMIT, &hpacket);
        ac_packet_set_data(hpacket, (const uint8_t *)buf, nread, sequence);
        uint32_t packet_size;
        const uint8_t *pkdata = ac_packet_serialize(hpacket, &packet_size);
        send(s, (const char *)&packet_size, sizeof(packet_size), 0);
        rc = send(s, (const char *)pkdata, packet_size, 0);
        ac_packet_free_serialized((void *)pkdata);
        ac_packet_destroy(hpacket);
        ac_free(buf);
        if (rc == SOCKET_ERROR)
        {
            flogf(stderr, "[error] failed to submit binary to remote sensor");
            conn_close(s);
            return INVALID_SOCKET;
        }
        sequence++;
    }
    fclose(file);
    return s;
}

#undef BIN_CHUNK

ac_error compile_rules(const char *path, ac_compiler **out)
{
    ac_error err;
    ac_compiler *compiler = ac_compiler_new();
    err = ac_compiler_add_file(compiler, path);
    if (err != ERROR_SUCCESS)
    {
        flogf(stderr, "[error] failed to use rule file %s (code 0x%x)", path, err);
        ac_compiler_free(compiler);
        return err;
    }
    if (out)
        *out = compiler;
    return err;
}

int run(const char *binpath, const char *rulePath, int mode, short localPort, const char *remoteAddr)
{
    SOCKET s;
    int rc;
    ac_packet_handle hpacket;
    uint32_t packet_size = 0;
    ac_packet_header hdr;
    ac_compiler *compiler;
    // compile the rules
    if (compile_rules(rulePath, &compiler) != ERROR_SUCCESS)
        return -1;

    if (mode == LOCAL_MODE)
        s = submit_local(localPort, (const uint8_t*)binpath, strlen(binpath));
    else
        s = submit_remote(remoteAddr, binpath);
    if (s == INVALID_SOCKET)
        return -1;
    rc = recv(s, (char *)&packet_size, sizeof(packet_size), 0);
    if (check_recv(s, rc) < 0)
    {
        conn_close(s);
        return -1;
    }
    uint8_t *pkdata = ac_alloc(packet_size);
    rc = recv(s, (char *)pkdata, packet_size, 0);
    if (check_recv(s, rc) < 0)
    {
        conn_close(s);
        ac_free(pkdata);
        return -1;
    }
    if (!ac_packet_read(pkdata, packet_size, &hpacket))
    {
        flogf(stderr, "[error] malformed packet received");
        conn_close(s);
        ac_free(pkdata);
        return -1;
    }
    ac_packet_get_header(hpacket, &hdr);
    if (hdr.magic != ALCA_MAGIC)
    {
        flogf(stderr, "[error] invalid magic number");
        conn_close(s);
        ac_free(pkdata);
        return -1;
    }
    if (hdr.version != ALCA_VERSION)
        flogf(stdout, "[warning] version mismatch between alca and sensor");
    if (hdr.data_type == AC_PACKET_DATA_SUBMIT_ERROR)
    {
        flogf(stderr, "[error] there was an error with binary submission, check sensor logs for more details");
        conn_close(s);
        ac_free(pkdata);
        return -1;
    }
    if (hdr.data_type != AC_PACKET_DATA_TRACE_START)
    {
        flogf(stderr, "[error] expected trace start notification (got 0x%x)", hdr.data_type);
        conn_close(s);
        ac_free(pkdata);
        return -1;
    }
    uint8_t *data = ac_alloc(hdr.data_len + 1);
    ac_packet_get_data(hpacket, data);
    char lportsz[6] = {0};
    itoa(localPort, lportsz, 10);
    flogf(stdout, "[info] connected to sensor: %s @ %s", data, mode == LOCAL_MODE ? lportsz : remoteAddr);
    rc = await(s, binpath);
    conn_close(s);
    ac_free(pkdata);
    return rc;
}

int main(int argc, char *argv[])
{
    cli_t *cli;
    CLI_BOOL help = 0;
    CLI_STRING binary = "";
    CLI_STRING ruleFile = "";
    CLI_INT localPort = 0;
    CLI_STRING remoteHost = "";
    int mode = LOCAL_MODE;
    cli_create("alca", "event-based rule engine for dynamic analysis and research", &cli);

    cli_add_arg(
        cli,
        CLI_ARG_BOOL | CLI_ARG_HELP,
        "--help",
        "-h",
        "show this help message and exit",
        0,
        (void **)&help);

    cli_add_arg(cli,
        CLI_ARG_STRING | CLI_ARG_POSITIONAL,
        "RULE_FILE",
        "",
        "rule file to run against the target",
        0,
        (void **)&ruleFile);

    cli_add_arg(cli,
        CLI_ARG_STRING | CLI_ARG_POSITIONAL,
        "TARGET",
        "",
        "binary to analyse",
        0,
        (void **)&binary);

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
        "(remote mode) connect to remote sensor at specified address (e.g. 127.1.1.1:8080)",
        0,
        (void **)&remoteHost);

    if (cli_parse_args(cli, argc, argv) < 0)
        return -1;

    if (help)
    {
        cli_print_help(cli);
        return -1;
    }
    if (strlen(remoteHost) == 0 && localPort == 0)
    {
        flogf(stderr, "[error] must specify either a local port or a remote host to connect to");
        return -1;
    }
    if (strlen(remoteHost) > 0 && localPort > 0)
    {
        flogf(stderr, "[error] must only specify one of --local or --remote-host");
        return -1;
    }
    if (strlen(remoteHost) > 0)
        mode = REMOTE_MODE;

    conn_api_init();
    run(binary, ruleFile, mode, localPort, remoteHost);
    conn_api_shutdown();
    cli_destroy(cli);
    return 0;
}
