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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sipc/sipc.h"
#include "sipc_internal.h"
#include "sipc_shm.h"
#include "sipc_mqueue.h"
#include "mqueue_internal.h"

#define SHM_RPERMS S_IRUSR|S_IRGRP|S_IROTH
#define SHM_SPERMS SHM_RPERMS|S_IWUSR|S_IWGRP|S_IWOTH
#define SHM_CPERMS SHM_RPERMS|SHM_SPERMS|IPC_CREAT|IPC_EXCL

static struct sipc_func_table shm_funcs = {
	.sipc_create = sipc_shm_create,
	.sipc_attach = sipc_shm_attach,
	.sipc_get_data_ptr = sipc_shm_get_data_ptr,
	.sipc_send_data = sipc_shm_send_data,
	.sipc_recv_data = sipc_shm_recv_data,
	.sipc_detach = sipc_shm_detach,
	._sipc_error = sipc_shm_error
};

int sipc_shm_init(sipc_t *sipc)
{
	if (!sipc)
		return -1;

	sipc->funcs = &shm_funcs;
	sipc->data = NULL;

	return 0;
}

int sipc_shm_create(sipc_t *sipc)
{
	if (!sipc)
		goto err;

	/* Create the shm segment */
	sipc->s.shmid = shmget(sipc->key, sipc->data_size, SHM_CPERMS);
	if (sipc->s.shmid < 0) {
		sipc_error(sipc, "shmget: %s\n", strerror(errno));
		goto err;
	}

	/* Create the side channel */
	sipc->s.msqid = mqueue_create(sipc->key);
	if (sipc->s.shmid < 0)
		goto err;

	/* Set capacity of side channel to only 1 message */
	if (mqueue_set_capacity(sipc->s.msqid, 1) < 0)
		goto err;

	return 0;

      err:
	if (sipc->s.shmid != (key_t) - 1)
		shmctl(sipc->s.shmid, IPC_RMID, NULL);
	return -1;
}

int sipc_shm_attach(sipc_t *sipc)
{
	int flags = 0;

	if (!sipc)
		goto err;

	/* Get the (existing) shm segment */
	flags = sipc->role == SIPC_SENDER ? SHM_SPERMS : SHM_RPERMS;
	sipc->s.shmid = shmget(sipc->key, sipc->data_size, flags);
	if (sipc->s.shmid < 0) {
		sipc_error(sipc, "shmget: %s\n", strerror(errno));
		goto err;
	}

	/* Attach to the shm segment */
	flags = sipc->role == SIPC_SENDER ? 0 : SHM_RDONLY;
	if ((sipc->data = shmat(sipc->s.shmid, NULL, flags)) == (char *)-1) {
		sipc_error(sipc, "shmat: %s\n", strerror(errno));
		goto err;
	}

	/* Connect side channel */
	sipc->s.msqid = mqueue_connect(sipc->key, sipc->role);
	if (sipc->s.msqid < 0) {
		sipc_error(sipc, "%s\n", "Unable to connect message queue");
		goto err;
	}

	return 0;
      err:
	sipc_shm_detach(sipc);
	return -1;
}

void sipc_shm_detach(sipc_t *sipc)
{
	if (!sipc)
		return;

	/* If we're the receiver, disconnect side channel */
	if (sipc->role == SIPC_RECEIVER)
		mqueue_disconnect(sipc->s.msqid);

	/* Detach shm segment */
	if (sipc->data) {
		if (shmdt(sipc->data) < 0)
			sipc_error(sipc, "shmdt: %s\n", strerror(errno));
	}
}

void sipc_shm_error(const char *fmt, va_list ap)
{
	vfprintf(stderr, fmt, ap);
}

char *sipc_shm_get_data_ptr(sipc_t *sipc)
{
	return sipc->data;
}

/* The order of signals from the writer to reader is DATA_READY,
 * DATA_READING, and then DATA_DONE.  The writer, running in a
 * different process than the reader, is to block between READY and
 * READING, and then between READING and DONE.  This is to prevent
 * race conditions where the writer modifies the shared memory buffer
 * before the reader is done with the data.
 */
int sipc_shm_send_data(sipc_t *sipc, size_t msg_len)
{
	int error;

	/* Send a DATA_READY marker */
	sipc->len = msg_len;
	if (mqueue_send_msg_type(sipc, sipc->s.msqid, SIPC_DATA_READY, MQ_BLOCK, msg_len) < 0) {
		error = errno;
		sipc_error(sipc, "Could not send DATA_READY marker\n");
		errno = error;
		return -1;
	}

	/* Send a DATA_READING marker; this should block until the
	   reader calls sipc_recv_data(). */
	if (mqueue_send_msg_type(sipc, sipc->s.msqid, SIPC_DATA_READING, MQ_BLOCK, 0) < 0) {
		error = errno;
		sipc_error(sipc, "Could not send DATA_READING marker\n");
		errno = error;
		return -1;
	}

	/* Send a DATA_DONE marker; this should block until the reader
	   calls sipc_shm_recv_done(). */
	if (mqueue_send_msg_type(sipc, sipc->s.msqid, SIPC_DATA_DONE, MQ_BLOCK, 0) < 0) {
		error = errno;
		sipc_error(sipc, "Could not send DATA_DONE marker\n");
		errno = error;
		return -1;
	}

	return 0;
}

/* Receive a DATA_READY marker from sender and pass the shm pointer to
 * the caller.  This function will block if there is nothing on the
 * message queue, if the handle is set to blocking mode (default).
 */
int sipc_shm_recv_data(sipc_t *sipc, char **data, size_t *len)
{
	int mtype = 0;
	int block;

	block = (sipc->non_blocking == 0 ? 1 : 0);

	/* Get a message from the side channel. */
	mtype = mqueue_get_msg_type(sipc, sipc->s.msqid, SIPC_ANY, block, len);
	if (mtype == SIPC_DATA_READY) {
		/* It is now OK to read shared memory */
		*data = sipc->data;
		return 0;
	} else {
		int error = errno;
		if (error == ENOMSG || error == EAGAIN)
			errno = EAGAIN;
		else {
			errno = error;
			sipc_error(sipc, "Received a message of unknown type\n");
		}
		return -1;
	}
}
