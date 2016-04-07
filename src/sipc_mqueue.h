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

#ifndef _SIPC_MQ_H_
#define _SIPC_MQ_H

#include <sipc/sipc.h>
#include <stdarg.h>

#define MQ_BLOCK 1
#define MQ_NOBLOCK 0

int sipc_mqueue_init(sipc_t *sipc);
int sipc_mqueue_connect(sipc_t *sipc);
void sipc_mqueue_disconnect(sipc_t *sipc);
char *sipc_mqueue_get_data_ptr(sipc_t *sipc);
int sipc_mqueue_send_data(sipc_t *sipc, size_t msg_len);
int sipc_mqueue_recv_data(sipc_t *sipc, char **data, size_t *len);
int sipc_mqueue_end_xmit(sipc_t *sipc);
void sipc_mqueue_destroy_handle(sipc_t *sipc);
void sipc_mqueue_error(const char *fmt, va_list ap);

#endif
