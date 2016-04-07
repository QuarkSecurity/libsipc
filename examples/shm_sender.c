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
* This is an example application which sends data to a shared memory 
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

/* Data file */ 
#define IN_FILE "data.txt"    

/* Amount of data to read from data file at a time;
 * also the amount of data to allocate inside the IPC handle */
#define READ_LEN 4096         

/* End of transmission marker which sender and receiver have agreed upon */
#define END_XMIT "0xDEADBEEF" 

/* Send an end of transmission marker */
static void send_end_xmit(sipc_t *sipc);

int main(void)
{
	int rbytes = 0;
	FILE *ifile = NULL;
	char *shm_data = NULL;

	/* Initialize the IPC handle */
	sipc_t *ipc = sipc_open(SIPC_KEY, SIPC_SENDER, SIPC_SYSV_SHM, READ_LEN);
	if (!ipc)
		goto err;       
	
	/* Get a pointer to data */
	shm_data = sipc_get_data_ptr(ipc);
	if (!shm_data) 
		goto err;	
	
	/* Open infile for reading */
	ifile = fopen(IN_FILE, "r");
	if (!ifile) {
		sipc_error(ipc, "fopen: %s\n", strerror(errno));
		goto err;
	}
	
	bzero(shm_data, READ_LEN);
	/* Read message-sized chunks at a time */
	while ((rbytes = fread(shm_data, sizeof(char), READ_LEN-1, ifile)) > 0) {	     	  
		if (sipc_send_data(ipc, rbytes) < 0)
			goto err;		
		
		if (feof(ifile))
			break;
		
		bzero(shm_data, READ_LEN);
	}
	send_end_xmit(ipc);

err:
	if (ifile)
		fclose(ifile);
	sipc_close(ipc);
	return 0;
}

/* Send an end of transmission marker. */
static void send_end_xmit(sipc_t *sipc)
{
	char *data = sipc_get_data_ptr(sipc);
	if (!data) 
		return;	

	bzero(data, READ_LEN);
	strncpy(data, END_XMIT, READ_LEN);
	sipc_send_data(sipc, READ_LEN);       
}
