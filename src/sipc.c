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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/ipc.h>
#include "sipc_internal.h"

int (*sipc_init_f[SIPC_NUM_TYPES]) (sipc_t *sipc) = {
sipc_shm_init, sipc_mqueue_init,};

sipc_t *sipc_open(const char *key, int role, int ipc_type, size_t len)
{
	sipc_t *new_sipc = NULL;
	key_t sipc_key;

	if (ipc_type >= SIPC_NUM_TYPES || ipc_type < 0)
		goto err;

	/* Create the key from a user supplied path */
	sipc_key = ftok(key, 'a');
	if ((int)sipc_key < 0) {
		fprintf(stderr, "ftok: %s\n", strerror(errno));
		goto err;
	}

	new_sipc = calloc(1, sizeof(sipc_t));
	if (!new_sipc)
		goto err;
	new_sipc->key = sipc_key;
	new_sipc->ipc_type = ipc_type;
	new_sipc->role = role;
	new_sipc->non_blocking = 0;
	new_sipc->data_size = len;
	new_sipc->len = 0;
	new_sipc->recv_len = 0;
	new_sipc->recv_data = NULL;

	/* call backend specific init to fill in function table */
	if (sipc_init_f[ipc_type] (new_sipc))
		goto err;

	/*
	 * Create the underlying ipc object or attach to an existing
	 * one depending on what role we are playing.
	 */
	switch (role) {
	case SIPC_CREATOR:
		if (new_sipc->funcs->sipc_create(new_sipc) < 0)
			goto err;
		break;
	case SIPC_SENDER:
	case SIPC_RECEIVER:
		if (new_sipc->funcs->sipc_attach(new_sipc) < 0)
			goto err;
		break;
	default:
		goto err;
	}

	return new_sipc;
      err:
	free(new_sipc);
	return NULL;
}

void sipc_close(sipc_t *sipc)
{
	if (!sipc)
		return;

	sipc->funcs->sipc_detach(sipc);
	free(sipc);
}

void sipc_unlink(const char *key, int ipc_type)
{
	key_t sipc_key;

	sipc_key = ftok(key, 'a');
	if ((int)(sipc_key) < 0) {
		fprintf(stderr, "ftok: %s\n", strerror(errno));
		return;
	}

	/* XXX - if we can require an sipc * we can avoid hard-coding */
	/* Choose which type of IPC resource to destroy */
	switch (ipc_type) {
	case SIPC_SYSV_MQUEUES:
		mqueue_destroy_resource(sipc_key);
		break;
	case SIPC_SYSV_SHM:
		mqueue_destroy_resource(sipc_key);
		shm_destroy_resource(sipc_key);
		break;
	}
}

int sipc_ioctl(sipc_t *sipc, int request)
{
	if (!sipc) {
		errno = EINVAL;
		return -1;
	}

	switch (request) {
	case SIPC_BLOCK:
		sipc->non_blocking = 0;
		break;
	case SIPC_NOBLOCK:
		sipc->non_blocking = IPC_NOWAIT;
		break;
	default:
		errno = EINVAL;
		return -1;
	}
	return 0;
}

char *sipc_get_data_ptr(sipc_t *sipc)
{
	if (!sipc)
		return NULL;

	return sipc->funcs->sipc_get_data_ptr(sipc);
}

int sipc_send_data(sipc_t *sipc, size_t msg_len)
{
	if (!sipc) {
		errno = EINVAL;
		return -1;
	}
	if (msg_len < 0) {
		errno = EINVAL;
		return -1;
	}
	if (sipc->role != SIPC_SENDER) {
		sipc_error(sipc, "sipc_send_data called without sender flag enabled in IPC structure\n");
		errno = EBADF;
		return -1;
	}
	if (msg_len > sipc->data_size) {
		sipc_error(sipc, "sipc_send_data: cannot send buffer of size %zu, can only receive %zd\n", msg_len, sipc->data_size);
		errno = ENOMEM;
		return -1;
	}

	return sipc->funcs->sipc_send_data(sipc, msg_len);
}

int sipc_recv_data(sipc_t *sipc, char **data, size_t *len)
{
	if (data)
		*data = NULL;	       /* Pointer to a buffer created here */
	if (len)
		*len = 0;	       /* Bytes received so far */
	if (!sipc || !data || !len) {
		errno = EINVAL;
		return -1;
	}
	if (sipc->role != SIPC_RECEIVER) {
		sipc_error(sipc, "sipc_recv_data called without receiver flag enabled in IPC structure\n");
		errno = EBADF;
		return -1;
	}

	return sipc->funcs->sipc_recv_data(sipc, data, len);
}

void sipc_error(sipc_t *sipc, const char *fmt, ...)
{
	if (!sipc)
		return;

	va_list ap;
	va_start(ap, fmt);
	sipc->funcs->_sipc_error(fmt, ap);
	va_end(ap);
}
