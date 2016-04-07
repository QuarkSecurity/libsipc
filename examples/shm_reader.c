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
* This is an example application which receives data from a shared memory 
* communications channel.   
*/

#include <stdio.h>
#include <stdlib.h>
#include <sipc/sipc.h>
#include <sys/ipc.h>
#include <string.h>
#include <errno.h>

/* Key which sender and receiver have agreed upon */
#define SIPC_KEY "sipc_shm_test"  

/* Amount of data to allocate inside the IPC handle */
#define READ_LEN 4096        

/* End of transmission marker which sender and receiver have agreed upon */ 
#define END_XMIT "0xDEADBEEF" 

/* Determine whether data contains an end of transmission marker */
static int is_end_xmit(char *data);

int main(void)
{
	char *data = NULL;
	size_t len = 0;

	/* Initialize the IPC handle */
	sipc_t *ipc = sipc_open(SIPC_KEY, SIPC_RECEIVER, SIPC_SYSV_SHM, READ_LEN);
	if (!ipc) {
		fprintf(stderr, "Could not create IPC resource\n");
		return -1;
	}

	/* Receive data from shared memory until an end of transmission
	 * marker has been received */
	while (!sipc_recv_data(ipc, &data, &len)) {
		if (is_end_xmit(data)) {
			sipc_shm_recv_done(ipc);
			break;
		}		     
		printf("%s", data);	     	     
		sipc_shm_recv_done(ipc);
	}
	
	sipc_close(ipc);
	return 0;
}

/* Determine whether data contains the end of transmission marker */
static int is_end_xmit(char *data)
{
	return !strcmp(data, END_XMIT);
}
