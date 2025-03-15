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

#ifndef AC_PACKET_H
#define AC_PACKET_H

#include <stdint.h>

#include <alca/utils.h>

#define AC_PACKET_LOCAL 0x1
#define AC_PACKET_REMOTE 0x2

#define AC_PACKET_DATA_LOCAL_SUBMIT 0x10
#define AC_PACKET_DATA_REMOTE_SUBMIT 0x11
#define AC_PACKET_DATA_SUBMIT_ERROR 0x12
#define AC_PACKET_DATA_TRACE_START 0x13 // sent by sensor to alca
#define AC_PACKET_DATA_EVENT 0x14 // for every event
#define AC_PACKET_DATA_TRACE_STOP 0x15 // sent by alca to stop event tracing
#define AC_PACKET_DATA_TRACE_END 0x16
#define AC_PACKET_MAX_RECV_SIZE 1024
#define AC_PACKET_SEQUENCE_LAST 0xffffffff

PACK(typedef struct ac_packet_header
{
    uint32_t magic;
    uint32_t version;
    uint32_t packet_type;
    uint32_t data_type;
    uint32_t data_len;
    uint32_t sequence;
}) ac_packet_header;

typedef void * ac_packet_handle;

AC_API int ac_packet_create(uint32_t packet_type, uint32_t data_type, ac_packet_handle *phandle);

AC_API int ac_packet_set_data(ac_packet_handle handle, const uint8_t *data, uint32_t data_len, uint32_t sequence_number);

AC_API uint8_t *ac_packet_serialize(ac_packet_handle handle, uint32_t *total_size);

AC_API void ac_packet_free_serialized(void *pkdata);

AC_API void ac_packet_destroy(ac_packet_handle handle);

AC_API int ac_packet_read(const uint8_t *data, uint32_t data_len, ac_packet_handle *phandle);

AC_API void ac_packet_get_header(ac_packet_handle handle, ac_packet_header *header);

AC_API void ac_packet_get_data(ac_packet_handle handle, uint8_t *data);

#endif //AC_PACKET_H
