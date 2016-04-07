/* Author: David Windsor <dwindsor@tresys.com>
*
* Copyright (C) 2006, 2007 Tresys Technology, LLC
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
*
* This is an example application which receives data from communications
* channel using a message queue. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sipc/sipc.h>
#include <sys/types.h>
#include <sys/ipc.h>

/* Key which sender and receiver have agreed upon */
#define SIPC_KEY "sipc_mq_test"

/* Amount of data to allocate inside the IPC handle */   
#define DATA_LEN 8192

/* End of message marker which sender and receiver have agreed upon */
#define DATA_END "0xDEADBEEF"  

/* Determine if the data received is the end of transmission marker */
static int END_XMIT(char *);

int main(void)
{
	size_t msglen = 0;
	char *data = NULL;

	/* Initialize the IPC handle */
	sipc_t *ipc = sipc_open(SIPC_KEY, SIPC_RECEIVER, SIPC_SYSV_MQUEUES, DATA_LEN);
	if (!ipc) {
		fprintf(stderr, "Error: unable to create IPC resource\n");
		return 1;
	}

	/* Receive data from shared memory until the end of transmission
	 * marker has been received. */
	while (!sipc_recv_data(ipc, &data, &msglen)) {
		if (END_XMIT(data))
			break;
		printf("%s", data);

		free(data);
		data = NULL;
	}

	/* Cleanup */
	sipc_close(ipc);
	return 0;
}

static int END_XMIT(char *data)
{
	if (!data)
		return 0;

	return !strcmp(data, DATA_END);
}
