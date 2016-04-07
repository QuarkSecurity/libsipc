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
//#include <sepol/policydb.h>

/* Key which sender and receiver have agreed upon */
#define SIPC_KEY "sipc_mq_test"

/* Amount of data to allocate inside the IPC handle */   
#define DATA_LEN 1048576

/* End of message marker which sender and receiver have agreed upon */
#define DATA_END "0xDEADBEEF"  

int main(int argc, char **argv)
{
	int msglen = 0;
	char *data = NULL;
	FILE *outfile;
	if (argc<2) {
	  printf("usage: %s <output file>\n",argv[0]);
	  exit(1);
	}

	outfile=fopen(argv[1],"wb");
	if (!outfile)
	  {
	    printf("Can't open output file %s\n",argv[1]);
	    exit(1);
	  }

	/* Create a new IPC handle for receiving */
	sipc_t *ipc = sipc_open(SIPC_KEY, SIPC_RECEIVER, SIPC_SYSV_MQUEUES, DATA_LEN);
	if (!ipc) {
		fprintf(stderr, "Error: unable to create IPC resource\n");
		return 1;
	}

	/* First receive the file */
	sipc_recv_data(ipc, &data, &msglen);
	printf("Received %d bytes.\n", msglen);
	/* Write it to disk */
	fwrite(data,1,msglen,outfile);
	fclose(outfile);

	printf("Successfully received policydb\n");

	/* Then receive the end of message marker */
	sipc_recv_data(ipc, &data, &msglen);
	
	/* Cleanup */
	sipc_close(ipc);
	return 0;
}
