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
* Libsipc was designed with the intention of minimizing backchannels 
* between reader and writer processes. Therefore, creation of IPC 
* resources must be controlled by an application outside of the communicating 
* processes. This is an example of such a helper application. 
* 
* Specifically, this helper application establishes the message queue side 
* channel for libsipc's shared memory backend.  
*/

#include <stdio.h>
#include <sipc/sipc.h>

/* This key is used to identify the IPC resource. It should be known
 * by both the sender and receiver. The file must already exist */
#define SIPC_KEY "sipc_shm_test"  

/* Amount of data to allocate inside the IPC handle */
#define READ_LEN 4096        
					   
int main(void)
{
	sipc_t *sipc;

	sipc = sipc_open(SIPC_KEY, SIPC_CREATOR, SIPC_SYSV_SHM, READ_LEN);
	if (sipc == NULL) {
		fprintf(stderr, "Unable to create message queue.\n");
		return -1;
	}
	sipc_close(sipc);

	return 0;
}
