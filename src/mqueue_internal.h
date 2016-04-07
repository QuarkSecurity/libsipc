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

#ifndef _MQUEUE_INTERNAL_H_
#define _MQUEUE_INTERNAL_H_

int mqueue_connect(key_t key, int sender);
void mqueue_disconnect(int msqid);
int mqueue_set_capacity(int msqid, int mqbytes);
int mqueue_get_msg_type(sipc_t *sipc, int msqid, int type, int block, size_t *payload);
int mqueue_send_msg_type(sipc_t *sipc, int msqid, int type, int block, size_t payload);
int mqueue_create(key_t key);
void mqueue_destroy_resource(key_t key);

#endif
