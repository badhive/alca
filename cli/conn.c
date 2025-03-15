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

#include "conn.h"

#include <stdint.h>

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
    int rc;
    SOCKET conn;
    struct addrinfo *result = NULL, hints;
    char portno[6] = {0};
    itoa(port, portno, 10);
    rc = getaddrinfo(address, portno, &hints, &result);
    if (rc != 0)
        return INVALID_SOCKET;
    conn = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (conn == INVALID_SOCKET)
    {
        goto end;
    }
    rc = connect(conn, result->ai_addr, (int)result->ai_addrlen);
    if (rc == SOCKET_ERROR)
    {
        closesocket(conn);
        conn = INVALID_SOCKET;
    }
end:
    freeaddrinfo(result);
    return conn;
}

void conn_close(SOCKET conn)
{
    shutdown(conn, 1);
    closesocket(conn);
}
