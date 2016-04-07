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
* This is an example application which sends data to communications 
* channel using a message queue.  
*/

#include <stdio.h>
#include <stdlib.h>
#include <sipc/sipc.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <string.h>
#include <errno.h>

/* Key which sender and receiver have agreed upon */
#define SIPC_KEY "sipc_mq_test"   

/* # of bytes we allocate for the IPC's internal buffer.
 * This should be >= the amount of data copied into the IPC */ 
#define IPC_LEN  8192          

/* # of bytes to read from data file at a time */
#define READ_LEN 4096
          
/* End of transmission marker which sender and receiver have agreed upon */
#define DATA_END "0xDEADBEEF"  

/* Data file */
#define IN_FILE  "data.txt"

/* Send an end of transmission marker */
static int send_end_xmit(sipc_t *ipc);  

int main(void)
{
	int retv = -1;
	FILE *ifile = NULL;
	size_t rbytes = 0;
	char *data = NULL;
	sipc_t *ipc = NULL;

	/* Initialize and connect IPC handle */
	ipc = sipc_open(SIPC_KEY, SIPC_SENDER, SIPC_SYSV_MQUEUES, IPC_LEN);
	if (!ipc) {
		fprintf(stderr, "Unable to create IPC resource\n");
		goto out;
	}

	/* Get pointer to the handle's internal buffer */
	data = sipc_get_data_ptr(ipc);
	if (!data) {
		sipc_error(ipc, "Unable to get data pointer\n");
		goto out;
	}

	/* Open data file for reading */
	ifile = fopen(IN_FILE, "r");
	if (!ifile) {
		sipc_error(ipc, "Unable to open data file\n");
		goto out;
	}
	
	/* Read READ_LEN bytes into the handle's internal buffer */
	while ((rbytes = fread(data, sizeof(char), READ_LEN, ifile)) > 0) {
		  /* Send this chunk of data */
		  if (sipc_send_data(ipc, IPC_LEN) < 0) 
			  sipc_error(ipc, "Unable to send IPC message\n");		  

		  if (feof(ifile))
			  break;

		  /* Clear out internal buffer */
		  bzero(data, IPC_LEN); 
	}
	
	/* Our messages have been sent; send end of transmission marker */
	send_end_xmit(ipc);
	retv = 0;
out:
	/* Cleanup */
	sipc_close(ipc);
	return retv;
}

/* Use this function to send an end of transmission marker */
/* Note that the receiver must be aware of the marker used here */
static int send_end_xmit(sipc_t *ipc)
{
	char *data = sipc_get_data_ptr(ipc);
	if (!data) {
		sipc_error(ipc, 
		    "Unable to get internal data pointer from IPC resource\n");
		return -1;
	}
	
	bzero(data, IPC_LEN);
	strncpy(data, DATA_END, IPC_LEN-1);
	if (sipc_send_data(ipc, IPC_LEN) < 0) {
		sipc_error(ipc, "Unable to send end of transmission marker\n");
		return -1;
	}

	printf("Send end of transmission marker: %s\n:", data);
	return 0;
}


