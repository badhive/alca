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

#ifndef AC_CONN_H
#define AC_CONN_H

#ifdef _WIN32
#ifndef _WIN32_WINNT // NOLINT(*-reserved-identifier)
#define _WIN32_WINNT 0x0501  /* Windows XP. */
#endif
#include <winsock2.h>
#else
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h> // close()
#include <netdb.h>  // getaddrinfo(), freeaddrinfo()
#endif
#include <cstdint>

int conn_api_init();

int conn_api_shutdown();

SOCKET conn_connect(const char *address, uint16_t port);

void conn_close(SOCKET conn);

#endif //AC_CONN_H
