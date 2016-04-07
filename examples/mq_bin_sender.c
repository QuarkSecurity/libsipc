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
#include <unistd.h>
#include <sipc/sipc.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>


/* Key which sender and receiver have agreed upon */
#define SIPC_KEY "sipc_mq_test"   

/* # of bytes we allocate for the IPC's internal buffer.
 * This should be >= the amount of data copied into the IPC */ 
#define IPC_LEN  1048576

/* # of bytes to read from data file at a time */
#define READ_LEN 4096
          
/* End of transmission marker which sender and receiver have agreed upon */
#define DATA_END "0xDEADBEEF"  

//#define POLICY_FILE "/etc/selinux/targeted/policy/policy.21"

/* Send an end of transmission marker */
static int send_end_xmit(sipc_t *ipc);  

int main(int argc, char **argv)
{
	int retv = -1;
	char *data = NULL, *file_data = NULL;
	int file_data_len = 0;
	sipc_t *ipc = NULL;
	if (argc<2) { printf("Usage is %s <input file>\n", argv[0]);
	  exit(1);
	}
			 
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

	int fd;
	struct stat sb;
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
	  fprintf(stderr, "Failed to open %s: %s\n", argv[1],
			strerror(errno));
		return -1;
	}
	if (fstat(fd, &sb) < 0) {
		fprintf(stderr, "Failed to fstat %s: %s\n",
			argv[1], strerror(errno));
		return -1;
	}

	file_data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (file_data == MAP_FAILED) {
	  fprintf(stderr, "Failed to mmap %s: %s\n", argv[1],
			strerror(errno));
		return -1;
	}

	file_data_len = sb.st_size;
	close(fd);

	/* Place the file into the handle's internal buffer */
	memcpy(data, file_data, file_data_len);

	/* Send the file */
	if (sipc_send_data(ipc, file_data_len) < 0) {
		sipc_error(ipc, "Unable to send message!\n");
		goto out;
	}

	printf("Sent %d bytes.\n", file_data_len);
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
	strncpy(data, DATA_END, strlen(DATA_END));
	if (sipc_send_data(ipc, strlen(DATA_END) + 1) < 0) {
		sipc_error(ipc, "Unable to send end of transmission marker\n");
		return -1;
	}

	return 0;
}
