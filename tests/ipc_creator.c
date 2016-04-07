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
*/

#include <stdio.h>
#include <stdlib.h>
#include <sipc/sipc.h>

#define DATA_LEN 8192

int main(int argc, char *argv[])
{
	sipc_t *sipc;

	if (argc != 3) {
		fprintf(stderr, "Usage: ipc_creator <key> <ipc type>\n");
		return -1;
	}

	sipc = sipc_open(argv[1], SIPC_CREATOR, atoi(argv[2]), DATA_LEN);
	if (sipc == NULL) {
		fprintf(stderr, "Unable to create ipc.\n");
		return -1;
	}

	return 0;
}
