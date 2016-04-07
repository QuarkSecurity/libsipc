/* Author: David Windsor <dwindsor@tresys.com>
*
* Copyright (C) 2006 - 2008 Tresys Technology, LLC
* Developed Under US JFCOM Sponsorship
*
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef _SIPC_SHM_H_
#define _SIPC_SHM_H_

#include <sipc/sipc.h>
#include <stdarg.h>

int sipc_shm_init(sipc_t *sipc);
int sipc_shm_connect(sipc_t *sipc);
void sipc_shm_disconnect(sipc_t *sipc);
char *sipc_shm_get_data_ptr(sipc_t *sipc);
int sipc_shm_send_data(sipc_t *sipc, size_t msg_len);
int sipc_shm_recv_data(sipc_t *sipc, char **data, size_t *len);
int sipc_shm_end_xmit(sipc_t *sipc);
void sipc_shm_destroy_handle(sipc_t *sipc);
void sipc_shm_error(const char *fmt, va_list ap);

#endif
