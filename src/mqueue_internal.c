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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "sipc/sipc.h"
#include "mqueue_internal.h"
#include "sipc_internal.h"

#define hidden __attribute__ ((visibility ("hidden")))

/* Everyone can r/w this queue, SELinux will protect it */
#define MQ_PERMS 0666

/* Create a message queue having the specified key.
 * This function should only be called by a helper application
 * and not by the sender or reciever. */
int mqueue_create(key_t key)
{
	int msqid;

	msqid = msgget(key, MQ_PERMS | IPC_CREAT | IPC_EXCL);
	if (msqid < 0)
		fprintf(stderr, "msgget: %s\n", strerror(errno));

	return msqid;
}

/* Destroy a message queue having the specified key.
 * This function should only be called by a helper application
 * and not by the sender or receiver. */
void mqueue_destroy_resource(key_t key)
{
	int msqid = 0;

	msqid = msgget(key, 0);
	if (msqid < 0) {
		fprintf(stderr, "msgget: %s\n", strerror(errno));
		return;
	}

	if (msgctl(msqid, IPC_RMID, NULL) < 0)
		fprintf(stderr, "msgctl: %s\n", strerror(errno));
}

int hidden mqueue_connect(key_t key, int role)
{
	int flags, msqid;

	switch (role) {
	case SIPC_SENDER:
		flags = 0200;
		break;
	case SIPC_RECEIVER:
		flags = 0400;
		break;
	default:
		return -1;
	}

	msqid = msgget(key, flags);
	if (msqid < 0) {
		fprintf(stderr, "msgget: %s\n", strerror(errno));
		return -1;
	}

	return msqid;
}

/* Do nothing here; disconnecting from the
 * message queue is done by a disconnector process */
void hidden mqueue_disconnect(int msqid)
{
}

/* Change the capacity of the message queue to contain a user
 * defined number of messages (not bytes) */
int hidden mqueue_set_capacity(int msqid, int mq_messages)
{
	struct msqid_ds mqbuf;

	if (msgctl(msqid, IPC_STAT, &mqbuf) < 0)
		return -1;

	/* Change the capacity of the queue */
	mqbuf.msg_qbytes = mq_messages * (sizeof(struct shm_msgbuf) - sizeof(long));
	if (msgctl(msqid, IPC_SET, &mqbuf) < 0)
		return -1;

	return 0;
}

/* Check a message queue to see if a message with given type exists.
 * If block is non-zero, this call will block if no such message exists in the
 * queue. */
int hidden mqueue_get_msg_type(sipc_t *sipc, int msqid, int type, int block, size_t *payload)
{
	struct shm_msgbuf *mbuf;
	int flags = block ? 0 : IPC_NOWAIT;
	int ret;

	mbuf = calloc(1, sizeof(*mbuf));
	if (!mbuf) {
		sipc_error(sipc, "Out of memory!\n");
		return -1;
	}	

	while (msgrcv(msqid, mbuf, sizeof(*mbuf) - sizeof(long), type, flags) < 0) {
		if (errno == EINTR) {
			continue;
		}
		if (errno == ENOMSG || errno == EAGAIN)
			return 0;
		return -1;
	}

	*payload = mbuf->payload;
	ret = mbuf->mtype;

	free(mbuf);

	return ret;
}

/* This exists solely for the shared memory implementation
 * to be able to send data-ready markers.
 * If block is non-zero, this call will block if the message queue is full. *
 * payload is currently used for packing the message length in the DATA_READY packet */
int hidden mqueue_send_msg_type(sipc_t *sipc, int msqid, int type, int block, size_t payload)
{
	struct shm_msgbuf *mbuf;
	int flags = block ? 0 : IPC_NOWAIT;

	mbuf = calloc(1, sizeof(*mbuf));
	if (!mbuf) {
		sipc_error(sipc, "Out of memory!\n");
		return -1;
	}	

	mbuf->mtype = type;
	mbuf->payload = payload;
	while (msgsnd(msqid, mbuf, sizeof(*mbuf) - sizeof(long), flags) < 0) {
		if (errno != EINTR) {
			return -1;
		}
	}

	free(mbuf);

	return 0;
}
