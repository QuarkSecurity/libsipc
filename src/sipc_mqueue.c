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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "sipc_internal.h"
#include "sipc_mqueue.h"
#include "mqueue_internal.h"

/* mqueue maximum message length */
int SIPC_MQUEUE_MSG_SZ;

/* procfs entry for max size of a message */
#define MQ_PROC "/proc/sys/fs/mqueue/msgsize_max"

static struct sipc_func_table mqueue_funcs = {
	.sipc_create = sipc_mqueue_create,
	.sipc_attach = sipc_mqueue_attach,
	.sipc_get_data_ptr = sipc_mqueue_get_data_ptr,
	.sipc_send_data = sipc_mqueue_send_data,
	.sipc_recv_data = sipc_mqueue_recv_data,
	.sipc_detach = sipc_mqueue_detach,
	._sipc_error = sipc_mqueue_error,
};

/* Forward decls */
static int get_max_msg(sipc_t *sipc);
static int mqueue_send_msg_len(sipc_t *sipc, size_t msg_len);
static int mqueue_send_end_xmit(sipc_t *sipc);
static int is_msg_len(struct msgbuf *mbuf);
static int is_end_xmit(struct msgbuf *mbuf);

int sipc_mqueue_init(sipc_t *sipc)
{
	if (!sipc)
		return -1;

	sipc->funcs = &mqueue_funcs;
	sipc->data = NULL;
	sipc->mbuf = NULL;

	/* Determine maximum size of a message from proc */
	SIPC_MQUEUE_MSG_SZ = get_max_msg(sipc);
	if (SIPC_MQUEUE_MSG_SZ < 0)
		return -1;

	return 0;
}

/* Create a message queue having the specified key.
 * This function should only be called by a helper application
 * and not by the sender or reciever. */
int sipc_mqueue_create(sipc_t *sipc)
{
	int msqid;

	msqid = mqueue_create(sipc->key);
	if (msqid < 0) {
		fprintf(stderr, "msgget: %s\n", strerror(errno));
		return -1;
	}

	sipc->msqid = msqid;
	return 0;
}

void sipc_mqueue_detach(sipc_t *sipc)
{
	free(sipc->data);
	sipc->data = NULL;
	free(sipc->mbuf);
	sipc->mbuf = NULL;
}

/* Attach to an existing message queue. */
int sipc_mqueue_attach(sipc_t *sipc)
{
	/* Allocate data member */
	sipc->data = calloc(1, sipc->data_size);
	if (!sipc->data) {
		sipc_error(sipc, "Out of memory\n");
		return -1;
	}

	/* Allocate message buffer */
	sipc->mbuf = (struct msgbuf *)calloc(1, SIPC_MQUEUE_MSG_SZ);
	if (!sipc->mbuf) {
		sipc_error(sipc, "Out of memory\n");
		return -1;
	}

	sipc->msqid = mqueue_connect(sipc->key, sipc->role);
	if (sipc->msqid < 0) {
		sipc_error(sipc, "msgget: %s\n", strerror(errno));
		free(sipc->data);
		sipc->data = NULL;
		free(sipc->mbuf);
		sipc->mbuf = NULL;
		return -1;
	}

	return 0;
}

char *sipc_mqueue_get_data_ptr(sipc_t *sipc)
{
	return sipc->data;
}

/* Sends a message to the queue.
 * Returns 0 on success, -1 on error */
int sipc_mqueue_send_data(sipc_t *sipc, size_t msg_len)
{
	size_t max_packet_sz = SIPC_MQUEUE_MSG_SZ - sizeof(long);
	int error = 0;

	struct msgbuf *mbuf = sipc->mbuf;
	assert(mbuf);
	memset(mbuf, 0, SIPC_MQUEUE_MSG_SZ);
	if (mqueue_send_msg_len(sipc, msg_len) < 0) {
		error = errno;
		goto err;
	}

	size_t amount_written = 0;
	while (amount_written < msg_len) {
		size_t amount_to_write = msg_len - amount_written > max_packet_sz ? max_packet_sz : msg_len - amount_written;

		mbuf->mtype = SIPC_DATA_READY;
		memcpy(mbuf->mtext, sipc->data + amount_written, amount_to_write);

		/* Send the message */
		while (msgsnd(sipc->msqid, mbuf, amount_to_write, 0) < 0) {
			error = errno;
			if (error != EINTR) {
				sipc_error(sipc, "msgsnd: %s\n", strerror(errno));
				goto err;
			}
		}
		amount_written += amount_to_write;
	}

	/* Send end of message marker */
	if (mqueue_send_end_xmit(sipc) < 0) {
		error = errno;
		goto err;
	}
	return 0;

      err:
	errno = error;
	return -1;
}

int sipc_mqueue_recv_data(sipc_t *sipc, char **data, size_t *len)
{
	int retv = -1;
	size_t idx, recv_sz;
	int error = 0;

	idx = 0;		       /* Current index into the buffer */

	struct msgbuf *mbuf = sipc->mbuf;
	assert(mbuf);
	memset(mbuf, 0, SIPC_MQUEUE_MSG_SZ);

	/* Receive and validate the length of message marker */
	if (sipc->recv_len == 0) {
		while (msgrcv(sipc->msqid, mbuf, SIPC_MQUEUE_MSG_SZ - sizeof(long), SIPC_ANY, sipc->non_blocking) < 0) {
			error = errno;
			if (error == EINTR) {
				continue;
			}
			if (error == ENOMSG || error == EAGAIN)
				error = EAGAIN;
			else
				sipc_error(sipc, "msgrcv: %s\n", strerror(errno));
			goto err;
		}
		if (is_end_xmit(mbuf)) {
			errno = 0;
			return 0;
		}

		if (is_msg_len(mbuf)) {
			errno = 0;
			sipc->recv_len = (size_t) strtoul(mbuf->mtext, NULL, 10);
			if (errno != 0) {
				error = EIO;
				sipc_error(sipc, "libsipc: bad message length\n");
				goto err;
			}
			if (sipc->recv_len > sipc->data_size) {
				error = EIO;
				sipc_error(sipc, "libsipc: cannot receive buffer of size %zd, can only receive %zd\n",
					   sipc->recv_len, sipc->data_size);
				goto err;
			}
		} else {
			error = EIO;
			sipc_error(sipc, "libsipc: did not receive length of message, received %ld\n", mbuf->mtype);
			goto err;
		}
		sipc->len = 0;
	}

	if (sipc->recv_len == 0) {
		/* special case if there is no data to be received: *data
		   needs to point to a buffer, but a malloc() of 0 is not
		   defined, so allocate a single byte for it. */
		if ((sipc->recv_data = malloc(1)) == NULL) {
			error = errno;
			sipc_error(sipc, "%s\n", "Out of memory");
			goto err;
		}
	}

	/* Receive the message payload.
	 * Stop when we have received the total number of bytes
	 * or upon receiving the end of message marker. */
	while ((recv_sz = msgrcv(sipc->msqid, mbuf, SIPC_MQUEUE_MSG_SZ - sizeof(long), SIPC_ANY, sipc->non_blocking)) < 0) {
		error = errno;
		if (error == EINTR) {
			continue;
		}
		if (error == ENOMSG || error == EAGAIN)
			error = EAGAIN;
		else
			sipc_error(sipc, "msgrcv: %s\n", strerror(errno));
		goto err;
	}

	while (!is_end_xmit(mbuf)) {
		size_t amount_to_copy = sipc->recv_len - sipc->len > recv_sz ? recv_sz : sipc->recv_len - sipc->len;
		if (amount_to_copy != 0) {
			char *tdata = realloc(sipc->recv_data, sipc->len + amount_to_copy);
			if (!tdata) {
				error = errno;
				sipc_error(sipc, "%s\n", "Out of memory");
				goto err;
			}
			sipc->recv_data = tdata;

			/* memset(*data+idx, 0x0, recv_sz); */
			memcpy(sipc->recv_data + sipc->len, mbuf->mtext, amount_to_copy);
			sipc->len += amount_to_copy;
		}

		/* Receive the next message */
		while ((recv_sz = msgrcv(sipc->msqid, mbuf, SIPC_MQUEUE_MSG_SZ - sizeof(long), SIPC_ANY, sipc->non_blocking)) < 0) {
			error = errno;
			if (error == EINTR) {
				continue;
			}
			if (error == ENOMSG || error == EAGAIN)
				error = EAGAIN;
			else
				sipc_error(sipc, "msgrcv: %s\n", strerror(errno));
			goto err;
		}
	}

	*data = sipc->recv_data;
	*len = sipc->len;
	sipc->recv_len = 0;
	sipc->len = 0;
	sipc->recv_data = NULL;
	retv = 0;

      err:
	if (retv && error != EAGAIN) {
		free(sipc->recv_data);
		sipc->recv_data = NULL;
		sipc->len = 0;
		sipc->recv_len = 0;
	}
	errno = error;
	return retv;
}

void sipc_mqueue_error(const char *fmt, va_list ap)
{
	vfprintf(stderr, fmt, ap);
}

/* Gets the maximum allowed size of a message in a message queue.
 * Returns the maximum allowed size of a message, -1 on error */
static int get_max_msg(sipc_t *sipc)
{
	FILE *f;
	char buf[1024];

	if (!(f = fopen(MQ_PROC, "r"))) {
		sipc_error(sipc, "fopen: %s\n", strerror(errno));
		return -1;
	}
	if (!fgets(buf, sizeof(buf), f)) {
		sipc_error(sipc, "fgets: %s\n", strerror(errno));
		(void)fclose(f);
		return -1;
	}
	if (fclose(f) == EOF)
		sipc_error(sipc, "fclose: %s\n", strerror(errno));

	return atoi(buf);
}

/* Send end of transmission marker */
static int mqueue_send_end_xmit(sipc_t *sipc)
{
	struct msgbuf mbuf;
	mbuf.mtype = SIPC_END_XMIT;
	memset(mbuf.mtext, 1, sizeof(mbuf.mtext));
	int error;

	while (msgsnd(sipc->msqid, (struct msgbuf *)&mbuf, sizeof(mbuf.mtext), 0) < 0) {
		error = errno;
		if (error != EINTR) {
			sipc_error(sipc, "msgsnd: %s\n", strerror(errno));
			errno = error;
			return -1;
		}
	}

	return 0;
}

/* Finds the length in digits of a number (approximates log10+1 for ints) */
static int qlog(int x)
{
	int i = 1;
	while (x) {
		i++;
		x /= 10;
	}
	return i;
}

static int mqueue_send_msg_len(sipc_t *sipc, size_t len)
{
	struct msgbuf *mbuf;
	int error;

	if (len < 0)
		return -1;
	/* msgbuf already has a byte in its mtext field, so no need to
	   add one here */
	mbuf = calloc(1, sizeof(struct msgbuf) + qlog(len));
	if (!mbuf) {
		sipc_error(sipc, "Out of memory!\n");
		return -1;
	}

	mbuf->mtype = SIPC_MSG_LEN;

	sprintf(mbuf->mtext, "%zu", len);

	while (msgsnd(sipc->msqid, mbuf, strlen(mbuf->mtext) + 1, 0) < 0) {
		error = errno;
		if (error != EINTR) {
			free(mbuf);
			sipc_error(sipc, "msgsnd: %s\n", strerror(errno));
			errno = error;
			return -1;
		}
	}
	free(mbuf);
	return 0;
}

/* Check message buffer to see if it contains an end of
 * transmission marker */
static int is_end_xmit(struct msgbuf *mbuf)
{
	if (mbuf->mtype == SIPC_END_XMIT)
		return 1;

	return 0;
}

static int is_msg_len(struct msgbuf *mbuf)
{
	if (mbuf->mtype == SIPC_MSG_LEN)
		return 1;

	return 0;
}
