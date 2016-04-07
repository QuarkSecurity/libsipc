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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <errno.h>
#include "sipc/sipc.h"
#include "sipc_internal.h"
#include "sipc_mqueue.h"
#include "mqueue_internal.h"

/* When called by the reader, this should clear out the message queue.
 * This will then unblock the writer, so that the writer can safely
 * modify the shared memory buffer.
 */
int sipc_shm_recv_done(sipc_t *sipc)
{
	size_t payload;

	if (!sipc || sipc->ipc_type != SIPC_SYSV_SHM) {
		errno = EINVAL;
		return -1;
	}

	if (mqueue_get_msg_type(sipc, sipc->s.msqid, SIPC_DATA_READING, MQ_BLOCK, &payload) < 0) {
		return -1;
	}
	sipc->len = 0;
	if (mqueue_get_msg_type(sipc, sipc->s.msqid, SIPC_DATA_DONE, MQ_BLOCK, &payload) < 0) {
		return -1;
	}
	return 0;
}

/* Destroy a shm segment having the specified key.
 * This function should only be called by a helper application
 * and not by the sender or receiver. */
void shm_destroy_resource(key_t sipc_key)
{
	int shmid = 0;

	/* Get the segment's ID */
	shmid = shmget(sipc_key, 0, 0);
	if (shmid < 0) {
		fprintf(stderr, "shmget: %s\n", strerror(errno));
		return;
	}

	/* Mark the segment for removal; this will actually
	   destroy the segment only if the sender and receiver have detached */
	if (shmctl(shmid, IPC_RMID, NULL) < 0)
		fprintf(stderr, "shmctl: %s\n", strerror(errno));

}
