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

#include <assert.h>
#include <stdio.h>
#include <alca/defaults.h>

#ifdef WIN32
#if defined(_MSC_VER)
#pragma comment(lib, "ws2_32.lib")
#endif
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

ac_error network_fn_nslookup_v4(ac_module *fn_object, ac_object *args, ac_object *result)
{
    int rc;
    const char *ip = args[0].s;
    const int port = args[1].i;
    char *host = ac_alloc(NI_MAXHOST);

    struct in_addr ip_addr = { 0 };
    if (inet_pton(AF_INET6, ip, &ip_addr) != 1)
        return AC_ERROR_UNSUCCESSFUL;

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr = ip_addr,
        .sin_port = htons(port),
    };
    rc = getnameinfo((struct sockaddr *)&addr,
        sizeof(addr),
        host,
        NI_MAXHOST,
        NULL,
        0,
        NI_NAMEREQD
        );
    if (rc != 0)
    {
        ac_free(host);
        return AC_ERROR_UNSUCCESSFUL;
    }
    result->s = host;
    return AC_ERROR_SUCCESS;
}

ac_error network_fn_nslookup_v6(ac_module *fn_object, ac_object *args, ac_object *result)
{
    int rc;
    const char *ip = args[0].s;
    const int port = args[1].i;

    struct in6_addr ip_addr6 = {0};
    if (inet_pton(AF_INET6, ip, &ip_addr6) != 1)
        return AC_ERROR_UNSUCCESSFUL;

    char *host = ac_alloc(NI_MAXHOST);
    struct sockaddr_in6 addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = ip_addr6,
        .sin6_port = htons(port),
    };
    rc = getnameinfo((struct sockaddr *)&addr,
        sizeof(addr),
        host,
        NI_MAXHOST,
        NULL,
        0,
        NI_NAMEREQD
        );
    if (rc != 0)
    {
        ac_free(host);
        return AC_ERROR_UNSUCCESSFUL;
    }
    result->s = host;
    return AC_ERROR_SUCCESS;
}

ac_module *ac_default_network_load_callback()
{
#if defined(WIN32)
    WSADATA wsa;
    assert(WSAStartup(MAKEWORD(2, 2), &wsa) == 0);
#endif
    enum network_event_action
    {
        NetAccept = 0,
        NetConnect = 1,
        NetDisconnect = 2,
        NetSend = 3,
        NetReceive = 4,
    };
    ac_module *module = ac_module_create("network", ALCA_VERSION, NULL);

    ac_module_add_field(module, "action", AC_FIELD_TYPE_INTEGER);

    ac_module_add_field(module, "tcp", AC_FIELD_TYPE_BOOLEAN);
    ac_module_add_field(module, "udp", AC_FIELD_TYPE_BOOLEAN);
    ac_module_add_field(module, "ipv6", AC_FIELD_TYPE_BOOLEAN);

    ac_module_add_field(module, "local_addr", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "local_port", AC_FIELD_TYPE_INTEGER);
    ac_module_add_field(module, "remote_addr", AC_FIELD_TYPE_STRING);
    ac_module_add_field(module, "remote_port", AC_FIELD_TYPE_INTEGER);

    ac_module_add_field(module, "packet_size", AC_FIELD_TYPE_INTEGER);

    ac_module *nslookup4 = ac_module_add_field(module, "nslookup4", AC_FIELD_TYPE_FUNCTION | AC_FIELD_TYPE_STRING);
    ac_module_set_function(nslookup4, network_fn_nslookup_v4, "si");
    ac_module *nslookup6 = ac_module_add_field(module, "nslookup6", AC_FIELD_TYPE_FUNCTION | AC_FIELD_TYPE_STRING);
    ac_module_set_function(nslookup6, network_fn_nslookup_v6, "si");

    ac_module_add_enum(module, NetAccept);
    ac_module_add_enum(module, NetConnect);
    ac_module_add_enum(module, NetDisconnect);
    ac_module_add_enum(module, NetSend);
    ac_module_add_enum(module, NetReceive);

    return module;
}

// developer-implemented
void ac_default_network_unload_callback(const ac_module *module)
{
}

// developer-implemented
int ac_default_network_unmarshal_callback(ac_module *module, const unsigned char *event_data, size_t size)
{
    return FALSE;
}
