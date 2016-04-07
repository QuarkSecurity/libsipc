/* Author: Joshua Brindle <jbrindle@tresys.com>
*          David Windsor <dwindsor@tresys.com>
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

#ifndef _SIPC_H_
#define _SIPC_H_
#include <sys/types.h>

struct sipc;
typedef struct sipc sipc_t;

/* SIPC roles */
#define SIPC_CREATOR 0
#define SIPC_SENDER 1
#define SIPC_RECEIVER 2

/* IPC types */
#define SIPC_SYSV_SHM 0
#define SIPC_SYSV_MQUEUES 1
#define SIPC_NUM_TYPES 2

/* SIPC behaviors, for sipc_ioctl() */
#define SIPC_BLOCK 0
#define SIPC_NOBLOCK 1

/* sipc_open(char *key, int role, int ipc_type, size_t size)
 *	key		Unique key to identify IPC communication channel,
 *			must be the same for the sender and reciever.
 *	role		SIPC_CREATOR if this application is the IPC creator,
 *			SIPC_SENDER for the sender and SIPC_RECEIVER
 *			for the reciever
 *	ipc_type	Which IPC type used, must be the same for sender
 *			and reciever.
 *	size		Maximum message size to be transmitted.
 *
 *	sipc_open must be called before any other function to initialize
 *	the sipc_t struct.  Caller must call sipc_close to free the memory.
 */
sipc_t *sipc_open(const char *key, int role, int ipc_type, size_t size);

/* Free the sipc struct and its contents */
void sipc_close(sipc_t *sipc);

/* Called by a helper application to destroy an IPC resource.
 * This function should *NOT* be called by normal users of the library. */
void sipc_unlink(const char *key, int ipc_type);

/* Modify the behavior of an open SIPC channel.  Returns 0 on success,
 * <0 on failure.
 */
int sipc_ioctl(sipc_t *sipc, int request);

/* Blocking call to send msglen bytes of data.
 * This can only be called if sender was specified when sipc_init was called.
 * returns 0 on success, <0 on failure */
int sipc_send_data(sipc_t *sipc, size_t msg_len);

/* Call to receive data.  Data will be allocated and filled and len
 * will be set to the length.  Returns 0 on success, <0 on failure. */
int sipc_recv_data(sipc_t *sipc, char **data, size_t *len);

/* Returns a pointer to the data contained within the IPC resource */
char *sipc_get_data_ptr(sipc_t *sipc);

/* Prints an error message, accepts printf format string */
void sipc_error(sipc_t *sipc, const char *fmt, ...)
	__attribute__ ((format(printf, 2, 3)));

/* Some IPC types unfortunatly need limited access to underlying mechanism */
/* shm specific functions */

/* Receiver calls this when it is done receiving a message in shared
 * memory.  There must be exactly one call to sipc_shm_recv_done() for
 * every call to sipc_recv_data(), if the sipc was created as shared
 * memory. */
int sipc_shm_recv_done(sipc_t *sipc);

#endif
