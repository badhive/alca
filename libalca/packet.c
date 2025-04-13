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
#include <alca/packet.h>

struct ac_packet
{
    ac_packet_header header;
    uint8_t *data;
};

int ac_packet_create(uint32_t packet_type, uint32_t data_type, ac_packet_handle *phandle)
{
    struct ac_packet *packet = ac_alloc(sizeof(struct ac_packet));
    packet->header.packet_type = netint(packet_type);
    packet->header.data_type = netint(data_type);
    packet->header.magic = netint(ALCA_MAGIC);
    packet->header.version = netint(ALCA_VERSION);
    if (!phandle)
    {
        return -1;
    }
    *phandle = packet;
    return 0;
}

int ac_packet_set_data(ac_packet_handle handle, const uint8_t *data, uint32_t data_len, uint32_t sequence_number)
{
    struct ac_packet *packet = handle;
    packet->data = ac_alloc(data_len);
    memcpy(packet->data, data, data_len);
    packet->header.data_len = data_len;
    packet->header.sequence = netint(sequence_number);
    return 0;
}

uint8_t *ac_packet_serialize(ac_packet_handle handle, uint32_t *total_size)
{
    struct ac_packet *packet = handle;
    uint32_t data_len = packet->header.data_len;
    packet->header.data_len = netint(data_len);

    uint8_t *pkdata = ac_alloc(sizeof(ac_packet_header) + data_len);
    memcpy(pkdata, &packet->header, sizeof(ac_packet_header));
    memcpy(pkdata + sizeof(ac_packet_header), packet->data, data_len);
    *total_size = sizeof(ac_packet_header) + data_len;
    return pkdata;
}

void ac_packet_destroy(ac_packet_handle handle)
{
    struct ac_packet *packet = handle;
    if (packet->data) ac_free(packet->data);
    ac_free(packet);
}

void ac_packet_free_serialized(void *pkdata)
{
    if (pkdata) ac_free(pkdata);
}

int ac_packet_read(const uint8_t *data, uint32_t data_len, ac_packet_handle *phandle)
{
    if (data_len < sizeof(ac_packet_header)) return -1;
    if (!phandle) return -1;
    ac_packet_handle handle;
    ac_packet_create(0, 0, &handle);
    uint32_t *ptr = (uint32_t *)data;
    struct ac_packet *packet = handle;
    packet->header.magic = netint(*ptr++);
    packet->header.version = netint(*ptr++);
    packet->header.packet_type = netint(*ptr++);
    packet->header.data_type = netint(*ptr++);
    packet->header.data_len = netint(*ptr++);
    packet->header.sequence = netint(*ptr++);
    if (packet->header.data_len > AC_PACKET_MAX_RECV_SIZE)
        return -1;
    packet->data = ac_alloc(packet->header.data_len);
    memcpy(packet->data, ptr, packet->header.data_len);
    *phandle = handle;
    return 0;
}

void ac_packet_get_header(ac_packet_handle handle, ac_packet_header *header)
{
    if (!header) return;
    struct ac_packet *packet = handle;
    *header = packet->header;
}

void ac_packet_get_data(ac_packet_handle handle, uint8_t *data)
{
    if (!data) return;
    struct ac_packet *packet = handle;
    memcpy(data, packet->data, packet->header.data_len);
}
