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

#include <csignal>
#include <cstdio>
#include <string>
#include <sys/stat.h>

#include <alca.h>
#include <CLI/CLI.hpp>
#include "conn.hpp"

#define LOCAL_MODE 1
#define REMOTE_MODE 2

SOCKET connectSocket = INVALID_SOCKET;

void flogf(FILE *fd, const char *format, ...)
{
    time_t rawtime;
    char time_str[80];

    time(&rawtime);
    tm *ti = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ti);
    fprintf(fd, "[%s] ", time_str);

    va_list args;
    va_start(args, format);
    vfprintf(fd, format, args);
    va_end(args);

    fprintf(fd, "\n");
}

void signalHandler(const int sig)
{
    if (sig == SIGINT)
    {
        flogf(stdout, "[info] shutting down...\n");
        if (connectSocket != INVALID_SOCKET)
        {
            conn_close(connectSocket);
        }
        conn_api_shutdown();
        exit(0);
    }
}

int check_recv(const int rc)
{
    if (rc == 0)
    {
        flogf(stderr, "[erro] connection to remote sensor has been closed");
        return -2;
    }
    if (rc < 0)
    {
        flogf(stderr, "[erro] failed to receive: %d", rc);
        return -1;
    }
    return 0;
}

struct vm_context
{
    const std::string& binPath;
};

void vm_print_trigger(const int type, char *name, const time_t at, void *ectx)
{
    std::vector<char> buffer (26);
    tm *tm_info = localtime(&at);
    std::string fmt = "[%s] [%s] [%s] name = \"%s\"\n";
    strftime(buffer.data(), 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf(fmt.c_str(), buffer, type == AC_VM_RULE ? "rule" : "sequ", static_cast<char *>(ectx), name);
}

void await(SOCKET s, const std::string& binPath, ac_vm *vm)
{
    int rc;
    int seq = 0;
    flogf(stdout, "[info] awaiting event data for %s, [ctrl+c] to exit...", binPath.c_str());
    vm_context ctx{binPath};
    while (true)
    {
        ac_packet_handle handle;
        // await data from socket
        uint32_t packet_size;
        rc = recv(s, reinterpret_cast<char *>(&packet_size), sizeof(packet_size), 0);
        if (check_recv(rc) < 0)
        {
            if (rc == -2)
            {
                conn_close(s);
                return;
            }
            continue;
        }
        packet_size = l2b(packet_size);
        if (!packet_size)
            continue;
        if (packet_size > AC_PACKET_MAX_RECV_SIZE)
        {
            flogf(stderr, "[erro] malformed packet received (exceeds maximum packet size)");
            continue;
        }
        std::vector<char> packet (packet_size);
        rc = recv(s, packet.data(), static_cast<int>(packet_size), 0);
        if ((rc = check_recv(rc)) < 0)
        {
            if (rc == -2)
            {
                conn_close(s);
                return;
            }
            continue;
        }
        if (!ac_packet_read(reinterpret_cast<const uint8_t *>(packet.data()), packet_size, &handle))
        {
            flogf(stderr, "[erro] malformed packet received");
            continue;
        }
        ac_packet_header hdr;
        ac_packet_get_header(handle, &hdr);
        if (hdr.magic != ALCA_MAGIC)
        {
            flogf(stderr, "[erro] seq-id=%d: invalid magic number", seq);
            continue;
        }
        if (hdr.version != ALCA_VERSION)
            flogf(stdout, "[warn] seq-id=%d: version mismatch between alca and sensor", seq);
        std::vector<uint8_t> data (hdr.data_len);
        ac_packet_get_data(handle, data.data());
        ac_vm_exec(vm, data.data(), hdr.data_len, &ctx);
        ac_packet_destroy(handle);
        seq++;
    }
}

SOCKET submit_local(uint16_t port, const uint8_t *data, uint32_t size)
{
    if (port <= 0 || port > 0xFFFF)
    {
        flogf(stderr, "[erro] invalid port number");
        return INVALID_SOCKET;
    }
    ac_packet_handle hpacket;
    int rc = 0;
    uint32_t packet_size;
    ac_packet_create(AC_PACKET_LOCAL, AC_PACKET_DATA_LOCAL_SUBMIT, &hpacket);
    ac_packet_set_data(hpacket, data, size, 0);
    void *packetData = ac_packet_serialize(hpacket, &packet_size);
    flogf(stdout, "[info] connecting to sensor @ localhost:%d ...", port);
    SOCKET s = conn_connect("localhost", port);
    if (s == INVALID_SOCKET)
    {
        flogf(stderr, "[erro] failed to connect to sensor");
        goto end;
    }
    packet_size = b2l(packet_size);
    send(s, reinterpret_cast<const char *>(&packet_size), sizeof(packet_size), 0);
    rc = send(s, static_cast<const char *>(packetData), static_cast<int>(packet_size), 0);
    if (rc == SOCKET_ERROR)
    {
        flogf(stderr, "[erro] failed to submit binary to sensor");
        conn_close(s);
        s = INVALID_SOCKET;
    }
end:
    ac_packet_free_serialized(packetData);
    ac_packet_destroy(hpacket);
    return s;
}

#define BIN_CHUNK 1024

SOCKET submit_remote(const std::string &host, const std::string &binpath)
{
    ac_packet_handle hpacket;
    int rc = 0;
    int port;
    int fsize;
    std::string sPort = host.substr(host.find(':') + 1, host.length() - 1);
    if (sPort.empty())
    {
        flogf(stderr, "[erro] invalid remote host address provided");
        return INVALID_SOCKET;
    }
    try
    {
        port = std::stoi(sPort);
    } catch (const std::logic_error &e)
    {
        flogf(stderr, "[erro] %s: invalid remote host address", e.what());
        return INVALID_SOCKET;
    }
    if (port <= 0 || port > 0xFFFF)
    {
        flogf(stderr, "[erro] invalid port number");
        return INVALID_SOCKET;
    }
    std::string hostname = host.substr(0, host.find(':'));
    if (hostname.empty())
    {
        flogf(stderr, "[erro] invalid remote host address");
        return INVALID_SOCKET;
    }
    flogf(stdout, "[info] connecting to sensor @ %s:%d ...", hostname.c_str(), port);
    SOCKET s = conn_connect(hostname.c_str(), port);
    if (s == INVALID_SOCKET)
    {
        flogf(stderr, "[erro] failed to connect to sensor");
        return s;
    }
    std::ifstream ifs;
    ifs.open(binpath);
    if (std::ifstream::failbit)
    {
        flogf(stderr, "[erro] could not open executable file %s: %s", binpath, strerror(errno));
        conn_close(s);
        return INVALID_SOCKET;
    }
    struct stat stat_info{};
    rc = stat(binpath.c_str(), &stat_info);
    if (rc != 0)
    {
        flogf(stderr, "[erro] could not get size of %s: %s", binpath, strerror(errno));
        conn_close(s);
        return INVALID_SOCKET;
    }
    fsize = stat_info.st_size;
    int bytesRead = 0;
    int sequence = 0;
    while (bytesRead < fsize)
    {
        std::vector<uint8_t> buf (BIN_CHUNK);
        ifs.seekg(bytesRead);
        int readSize = fsize - bytesRead < BIN_CHUNK ? fsize - bytesRead : BIN_CHUNK;
        ifs.read(reinterpret_cast<char *>(buf.data()), readSize);
        bytesRead += readSize;

        if (bytesRead >= fsize)
            sequence = AC_PACKET_SEQUENCE_LAST;
        ac_packet_create(AC_PACKET_REMOTE, AC_PACKET_DATA_REMOTE_SUBMIT, &hpacket);
        ac_packet_set_data(hpacket, buf.data(), readSize, sequence);

        uint32_t packetSize;
        void *packetData = ac_packet_serialize(hpacket, &packetSize);
        packetSize = b2l(packetSize);
        send(s, reinterpret_cast<const char *>(packetSize), sizeof(packetSize), 0);
        rc = send(s, static_cast<const char *>(packetData), static_cast<int>(packetSize), 0);

        ac_packet_free_serialized(packetData);
        ac_packet_destroy(hpacket);
        if (rc == SOCKET_ERROR)
        {
            flogf(stderr, "[erro] failed to submit binary to remote sensor");
            conn_close(s);
            return INVALID_SOCKET;
        }
        sequence++;
    }
    return s;
}

#undef BIN_CHUNK

ac_error compile_rules(ac_compiler *compiler, const std::vector<std::string> &paths)
{
    for (const auto &path: paths)
    {
        ac_error err = ac_compiler_add_file(compiler, path.c_str());
        if (err != ERROR_SUCCESS)
        {
            flogf(stderr, "[erro] failed to use rule file %s (code 0x%x)", path, err);
            ac_compiler_free(compiler);
            return err;
        }
    }
    return ERROR_SUCCESS;
}

int run(const std::string &binpath, const std::vector<std::string> &rulePaths, int mode, uint16_t localPort,
        const std::string &remoteAddr)
{
    SOCKET s;
    int rc;
    ac_packet_handle hpacket;
    uint32_t packetSize = 0;
    ac_packet_header hdr;
    ac_compiler *compiler = ac_compiler_new();
    // compile the rules
    if (compile_rules(compiler, rulePaths) != ERROR_SUCCESS)
        return -1;
    ac_vm *vm = ac_vm_new(compiler);
    if (mode == LOCAL_MODE)
        s = submit_local(localPort, reinterpret_cast<const uint8_t *>(binpath.data()), binpath.length());
    else
        s = submit_remote(remoteAddr, binpath);
    if (s == INVALID_SOCKET)
        return -1;
    rc = recv(s, reinterpret_cast<char *>(&packetSize), sizeof(packetSize), 0);
    if (check_recv(rc) < 0)
    {
        conn_close(s);
        return -1;
    }
    packetSize = l2b(packetSize);
    if (packetSize > AC_PACKET_MAX_RECV_SIZE)
    {
        flogf(stderr, "[erro] malformed packet received (exceeds maximum packet size)");
        conn_close(s);
        return -1;
    }
    std::vector<char> packetData (packetSize);
    rc = recv(s, packetData.data(), static_cast<int>(packetSize), 0);
    if (check_recv(rc) < 0)
    {
        conn_close(s);
        return -1;
    }
    if (!ac_packet_read(reinterpret_cast<const uint8_t *>(packetData.data()), packetSize, &hpacket))
    {
        flogf(stderr, "[erro] malformed packet received");
        conn_close(s);
        return -1;
    }
    ac_packet_get_header(hpacket, &hdr);
    if (hdr.magic != ALCA_MAGIC)
    {
        flogf(stderr, "[erro] invalid magic number");
        conn_close(s);
        return -1;
    }
    if (hdr.version != ALCA_VERSION)
        flogf(stdout, "[warning] version mismatch between alca and sensor");
    if (hdr.data_type == AC_PACKET_DATA_SUBMIT_ERROR)
    {
        flogf(stderr, "[erro] there was an error with binary submission, check sensor logs for more details");
        conn_close(s);
        return -1;
    }
    if (hdr.data_type != AC_PACKET_DATA_TRACE_START)
    {
        flogf(stderr, "[erro] expected trace start notification (got 0x%x)", hdr.data_type);
        conn_close(s);
        return -1;
    }
    std::vector<uint8_t> data (hdr.data_len);
    ac_packet_get_data(hpacket, data.data());
    std::string sPort = std::to_string(localPort);
    flogf(stdout, "[info] connected to sensor: %s @ %s", data, mode == LOCAL_MODE ? sPort : remoteAddr);
    signal(SIGINT, signalHandler);
    await(s, binpath, vm);
    conn_close(s);
    return rc;
}

int main(int argc, char *argv[])
{
    std::string binary, remoteHost;
    std::vector<std::string> ruleFiles;
    uint16_t localPort = 4164;
    int mode = LOCAL_MODE;

    CLI::App app("event-based rule engine for dynamic analysis and research", "alca");
    argv = app.ensure_utf8(argv);

    app.add_option(
        "target",
        binary,
        "executable to analyse");
    app.add_option<std::vector<std::string> >(
        "--rules",
        ruleFiles,
        "rule file(s) to run against the target")->required();
    app.add_option(
        "-r,--remote",
        remoteHost,
        "(remote mode) connect to remote sensor at specified address (e.g. 127.1.1.1:8080)");
    auto opLPort = app.add_option(
        "-l,--local",
        localPort,
        "(local mode, default) connect to sensor on the specified port");
    CLI11_PARSE(app, argc, argv);

    if (!remoteHost.empty() && opLPort->count() > 0)
    {
        flogf(stderr, "[erro] cannot specify both --local and --remote");
        return -1;
    }
    if (!remoteHost.empty())
        mode = REMOTE_MODE;

    conn_api_init();
    run(binary, ruleFiles, mode, localPort, remoteHost);
    conn_api_shutdown();
    return 0;
}
