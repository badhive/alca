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

#include <string>
#include <Ws2tcpip.h>

#include "conn.hpp"

#ifdef _WIN32
#define conn_error() WSAGetLastError()
#else
#define conn_error() errno
#endif

int last_error = 0;

int conn_api_init()
{
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa);
#else
    return 0;
#endif
}

int conn_api_shutdown()
{
#ifdef _WIN32
    return WSACleanup();
#else
    return 0;
#endif
}

SOCKET conn_connect(const char *address, uint16_t port)
{
    last_error = 0;
    addrinfo *result = nullptr;
    addrinfo hints = { 0 };
    hints.ai_family = AF_INET;
    int rc = getaddrinfo(address, std::to_string(port).c_str(), &hints, &result);
    if (rc != 0)
    {
        last_error = rc;
        return INVALID_SOCKET;
    }
    SOCKET conn = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (conn == INVALID_SOCKET)
    {
        last_error = conn_error();
        goto end;
    }
    rc = connect(conn, result->ai_addr, static_cast<int>(result->ai_addrlen));
    if (rc == SOCKET_ERROR)
    {
        last_error = conn_error();
        closesocket(conn);
        conn = INVALID_SOCKET;
    }
end:
    freeaddrinfo(result);
    return conn;
}

int conn_last_error()
{
    return last_error;
}

void conn_close(SOCKET conn)
{
    shutdown(conn, 1);
    closesocket(conn);
    last_error = conn_error();
}
