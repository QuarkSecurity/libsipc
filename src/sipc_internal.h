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

#include <sipc/sipc.h>
#include <stdarg.h>
#include "sipc_mqueue.h"
#include "sipc_shm.h"

#define hidden __attribute__ ((visibility ("hidden")))

/* Control message types */
#define SIPC_ANY	0x00
#define SIPC_DATA_READY		0x03
#define SIPC_DATA_READING	0x04
#define SIPC_DATA_DONE		0x05
#define SIPC_MSG_LEN	0x09
#define SIPC_END_XMIT	0x0c

/* msgbuf wrapper used for shm control messages */
struct shm_msgbuf {
	long mtype; /* Control message type */
	size_t payload; /* Currently used for packing len */
}; 

/* Creation functions */
int sipc_mqueue_create(sipc_t *sipc);
int sipc_shm_create(sipc_t *sipc);
int mqueue_create(key_t key);

/* Attach functions */
int sipc_mqueue_attach(sipc_t *sipc);
int sipc_shm_attach(sipc_t *sipc);

/* Detach functions */
void sipc_mqueue_detach(sipc_t *sipc);
void sipc_shm_detach(sipc_t *sipc);

/* Destruction functions */
void mqueue_destroy_resource(key_t key);
void shm_destroy_resource(key_t key);

/* Initialization functions */
int sipc_mqueue_init(sipc_t *sipc);
int sipc_shm_init(sipc_t *sipc);

/* all ipc backends must implement all of these functions */
struct sipc_func_table
{
	int (*sipc_create) (sipc_t *sipc);
	int (*sipc_attach) (sipc_t *sipc);
	char *(*sipc_get_data_ptr) (sipc_t *sipc);
	int (*sipc_send_data) (sipc_t *sipc, size_t msg_len);
	int (*sipc_recv_data) (sipc_t *sipc, char **data, size_t *len);
	void (*sipc_detach) (sipc_t *sipc);
	void (*_sipc_error) (const char *fmt, va_list ap);
};

struct sipc
{
	key_t key;
	int ipc_type;
	int role;
	int non_blocking;	       /* set to IPC_NOWAIT if reads should be
				          nonblocking, 0 to block (default) */
	union
	{
		int fd;		       /* file desciptor for socket connections */
		struct shm
		{		       /* shm data */
			int shmid;     /* ID of segment */
			int msqid;     /* notify channel */
			/* sipc_t *mqueue; handle to notify channel */
		} s;
		int msqid;	       /* message queue */
	};

	char *data;		       /* Buffer for sending data */
	struct msgbuf *mbuf;
	size_t data_size;	       /* Size of the data buffer, set in sipc_open() */
	size_t len;		       /* Number of bytes xmitted so far,
				          or number of bytes received so far */
	size_t recv_len;	       /* Number of bytes to be received */
	char *recv_data;	       /* Buffer for receiving mqueue data */
	struct sipc_func_table *funcs;
};
