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

#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <alca.h>
#include "cli.h"
#include "conn.h"

#define LOCAL_MODE 1
#define REMOTE_MODE 2

SOCKET connectSocket = INVALID_SOCKET;

void flogf(FILE *fd, const char *format, ...) {
    time_t rawtime;
    char time_str[80];

    time(&rawtime);
    struct tm *ti = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ti);
    fprintf(fd, "[%s] ", time_str);

    va_list args;
    va_start(args, format);
    vfprintf(fd, format, args);
    va_end(args);

    fprintf(fd, "\n");
}

void signalHandler(int sig)
{
    if (connectSocket != INVALID_SOCKET)
    {
        conn_close(connectSocket);
    }
}

int check_recv(SOCKET s, int rc)
{
    if (rc == 0)
    {
        flogf(stderr, "[error] connection to remote sensor has been closed");
        conn_close(s);
        return -2;
    }
    if (rc < 0)
    {
        flogf(stderr, "[error] failed to receive: %d", rc);
        conn_close(s);
        return -1;
    }
    return 0;
}

void vm_print_trigger(int type, char *name, const time_t at, void *ectx)
{
    char buffer[26] = {0};
    struct tm *tm_info = localtime(&at);
    const char *fmt = "[%s] [%s] [%s] name = \"%s\"\n";
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf(fmt, buffer, type == AC_VM_RULE ? "rule" : "sequ", (char *)ectx, name);
}

void await(SOCKET s, const char *binPath, ac_vm *vm)
{
    int rc;
    int seq = 0;
    flogf(stdout, "[info] awaiting event data for %s, [ctrl+c] to exit...", binPath);
    while (1)
    {
        ac_packet_handle handle;
        // await data from socket
        uint32_t packet_size;
        rc = recv(s, (char *)&packet_size, sizeof(packet_size), 0);
        if (check_recv(s, rc) < 0)
        {
            if (rc == -2)
                return;
            continue;
        }
        packet_size = l2b(packet_size);
        if (!packet_size)
            continue;
        if (packet_size > AC_PACKET_MAX_RECV_SIZE)
        {
            flogf(stderr, "[error] malformed packet received (exceeds maximum packet size)");
            continue;
        }
        uint8_t *packet = ac_alloc(packet_size);
        rc = recv(s, (char *)packet, (int)packet_size, 0);
        if ((rc = check_recv(s, rc)) < 0)
        {
            ac_free(packet);
            if (rc == -2)
                return;
            continue;
        }
        if (!ac_packet_read(packet, packet_size, &handle))
        {
            flogf(stderr, "[error] malformed packet received");
            ac_free(packet);
            continue;
        }
        ac_packet_header hdr;
        ac_packet_get_header(handle, &hdr);
        if (hdr.magic != ALCA_MAGIC)
        {
            flogf(stderr, "[error-%d] invalid magic number", seq);
            ac_free(packet);
            continue;
        }
        if (hdr.version != ALCA_VERSION)
            flogf(stdout, "[warning-%d] version mismatch between alca and sensor", seq);
        uint8_t *data = ac_alloc(hdr.data_len);
        ac_packet_get_data(handle, data);
        ac_vm_exec(vm, data, hdr.data_len, (void *)binPath);
        ac_packet_destroy(handle);
        ac_free(packet);
        seq++;
    }
}

SOCKET submit_local(uint16_t port, const uint8_t *data, uint32_t size)
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
    rc = send(s, (const char *)pkdata, (int)packet_size, 0);
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
    char *hostname = ac_alloc(pport - host + 1);
    memcpy(hostname, host, pport - host + 1);
    hostname[pport - host] = '\0';
    SOCKET s = conn_connect(hostname, port);
    if (s == INVALID_SOCKET)
    {
        flogf(stderr, "[error] failed to connect to remote host %s:%d", hostname, port);
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
        rc = send(s, (const char *)pkdata, (int)packet_size, 0);
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

int run(const char *binpath, const char *rulePath, int mode, uint16_t localPort, const char *remoteAddr)
{
    SOCKET s;
    int rc;
    ac_packet_handle hpacket;
    uint32_t packet_size = 0;
    ac_packet_header hdr;
    ac_compiler *compiler;
    ac_vm *vm;
    // compile the rules
    if (compile_rules(rulePath, &compiler) != ERROR_SUCCESS)
        return -1;
    vm = ac_vm_new(compiler);
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
    packet_size = l2b(packet_size);
    if (packet_size > AC_PACKET_MAX_RECV_SIZE)
    {
        flogf(stderr, "[error] malformed packet received (exceeds maximum packet size)");
        conn_close(s);
        return -1;
    }
    uint8_t *pkdata = ac_alloc(packet_size);
    rc = recv(s, (char *)pkdata, (int)packet_size, 0);
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
    ac_free(pkdata);
    signal(SIGINT, signalHandler);
    await(s, binpath, vm);
    conn_close(s);
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
    run(binary, ruleFile, mode, (uint16_t)localPort, remoteHost);
    conn_api_shutdown();
    cli_destroy(cli);
    return 0;
}
