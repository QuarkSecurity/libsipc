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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <sipc/sipc.h>
#include <CUnit/Basic.h>
#include <sipc/sipc.h>
#include "shm.h"

#define SIPC_SHM_KEY "shm_ipc_test"

#define CREATOR_APP "ipc_creator"
#define DESTROYER_APP "ipc_destroyer"
#define IN_FILE "data"
#define DATA_LEN 4096
#define DATA_END "0xDEADBEEF"

static sipc_t *writer_ipc;
static sipc_t *reader_ipc;
static void do_child(void);
static void do_parent(void);
static void do_binary_child(void);
static void do_binary_parent(void);
static void do_blocked_child(void);
static void do_blocked_parent(void);
static void send_end_xmit(sipc_t *sipc);
static int verify_data(char *filename, char *recv_data);
static inline int END_XMIT(char *data);

int test_shm_init(void)
{
	pid_t pid;
	int status;
	char ipc_type_str[128];

	/* Build the creator app's argv */
	bzero(ipc_type_str, 128);
	snprintf(ipc_type_str, 128, "%d", SIPC_SYSV_SHM);
	char *helper_argv[] = {
		CREATOR_APP,
		SIPC_SHM_KEY,
		ipc_type_str,
		NULL,
	};

	/* Run helper app to create a message queue for our side channel */
	switch ((pid = fork())) {
	case -1:
		fprintf(stderr, "fork(): %s\n", strerror(errno));
		return -1;
	case 0:
		execve(CREATOR_APP, helper_argv, NULL);
		break;
	default:
		break;
	}
	wait(&status);

	/* Open IPCs */
	writer_ipc = sipc_open(SIPC_SHM_KEY, SIPC_SENDER, SIPC_SYSV_SHM, DATA_LEN);
	if (writer_ipc == NULL) {
		fprintf(stderr, "Could not initialize IPC resource\n");
		return -1;
	}
	reader_ipc = sipc_open(SIPC_SHM_KEY, SIPC_RECEIVER, SIPC_SYSV_SHM, DATA_LEN);
	if (reader_ipc == NULL) {
		fprintf(stderr, "Could not initialize IPC resource\n");
		return -1;
	}

	return 0;
}

int test_shm_cleanup(void)
{
	int status;
	pid_t pid;
	char ipc_type_str[128];

	sipc_close(reader_ipc);
	sipc_close(writer_ipc);

	/* Build the destroyer app's argv */
	bzero(ipc_type_str, 128);
	snprintf(ipc_type_str, 128, "%d", SIPC_SYSV_SHM);
	char *helper_argv[] = {
		DESTROYER_APP,
		SIPC_SHM_KEY,
		ipc_type_str,
		NULL,
	};

	/* Run helper app to destroy the shm segment and message queue */
	switch ((pid = fork())) {
	case -1:
		fprintf(stderr, "fork(): %s\n", strerror(errno));
		return -1;
	case 0:
		execve(DESTROYER_APP, helper_argv, NULL);
		break;
	default:
		break;
	}
	wait(&status);

	return 0;
}

static void test_shm(void)
{
	int status;
	pid_t pid;

	switch (pid = fork()) {
	case -1:
		fprintf(stderr, "fork: %s\n", strerror(errno));
		return;
	case 0:
		sipc_close(reader_ipc);
		do_child();
		/* Exit here to prevent displaying results 2x */
		exit(0);
		break;
	default:
		do_parent();
		break;
	}

	wait(&status);
}

static void test_binary_shm(void)
{
	int status;
	pid_t pid;

	switch (pid = fork()) {
	case -1:
		fprintf(stderr, "fork: %s\n", strerror(errno));
		return;
	case 0:
		sipc_close(reader_ipc);
		do_binary_child();
		/* Exit here to prevent displaying results 2x */
		exit(0);
		break;
	default:
		do_binary_parent();
		break;
	}

	wait(&status);
}

static void test_blocked_shm(void)
{
	int status;
	pid_t pid;

	switch (pid = fork()) {
	case -1:
		fprintf(stderr, "fork: %s\n", strerror(errno));
		return;
	case 0:
		sipc_close(reader_ipc);
		do_blocked_child();
		/* Exit here to prevent displaying results 2x */
		exit(0);
		break;
	default:
		do_blocked_parent();
		break;
	}

	wait(&status);
}

CU_TestInfo shm_tests[] = {
	{"forked shm", test_shm}
	,
	{"binary shm", test_binary_shm}
	,
	{"blocked shm", test_blocked_shm}
	,
	CU_TEST_INFO_NULL
};

static void do_child(void)
{
	int rbytes = 0;
	char *shm_data = NULL;
	FILE *ifile = NULL;

	/* Get pointer to IPC's internal data */
	shm_data = sipc_get_data_ptr(writer_ipc);
	if (!shm_data) {
		sipc_error(writer_ipc, "Unable to get pointer to data.\n");
		return;
	}

	if ((ifile = fopen(IN_FILE, "r")) == NULL) {
		sipc_error(writer_ipc, "fopen: %s\n", strerror(errno));
		return;
	}
	/* Read DATA_LEN bytes from file */
	bzero(shm_data, DATA_LEN);
	if ((rbytes = fread(shm_data, sizeof(char), DATA_LEN - 1, ifile)) < 0) {
		sipc_error(writer_ipc, "fread: %s\n", strerror(errno));
		fclose(ifile);
		return;
	}
	fclose(ifile);

	/* Send the data */
	if (sipc_send_data(writer_ipc, DATA_LEN) < 0) {
		sipc_error(writer_ipc, "Unable to send data\n");
		return;
	}
	send_end_xmit(writer_ipc);
	CU_PASS("Successfully sent data via SYSV shared memory");

	sipc_close(writer_ipc);
}

static void do_parent(void)
{
	char *data = NULL;
	char recv_data[DATA_LEN];
	size_t len = 0;

	memset(recv_data, 0, sizeof(recv_data));
	while (!sipc_recv_data(reader_ipc, &data, &len)) {
		CU_ASSERT_PTR_NOT_NULL_FATAL(data);
		if (END_XMIT(data)) {
			sipc_shm_recv_done(reader_ipc);
			break;
		}

		/* Copy received data for verification later */
		memcpy(recv_data, data, len);
		sipc_shm_recv_done(reader_ipc);
	}
	CU_PASS("Successfully read a message via SYSV shared memory");

	if (verify_data(IN_FILE, recv_data) != 1) {
		CU_PASS("Sent data matches received data");
	} else {
		CU_FAIL("Sent data does not match received data!");
	}
}

/*
 * Write a sequence of bytes to the shm SIPC.
 */
static void do_binary_child(void)
{
	size_t i = 0;
	char *shm_data = NULL;

	/* Get pointer to IPC's internal data */
	shm_data = sipc_get_data_ptr(writer_ipc);
	if (!shm_data) {
		sipc_error(writer_ipc, "Unable to get pointer to data.\n");
		return;
	}

	for (i = 0; i < DATA_LEN; i++) {
		shm_data[i] = i % 256;
	}

	/* Send the data */
	if (sipc_send_data(writer_ipc, DATA_LEN) < 0) {
		sipc_error(writer_ipc, "Unable to send data\n");
		return;
	}
	send_end_xmit(writer_ipc);
	CU_PASS("Successfully sent binary data via SYSV shared memory");

	sipc_close(writer_ipc);
}

static void do_binary_parent(void)
{
	char *data = NULL;
	unsigned char recv_data[DATA_LEN];
	size_t len = 0, i;

	memset(recv_data, 0, sizeof(recv_data));
	while (!sipc_recv_data(reader_ipc, &data, &len)) {
		if (END_XMIT(data)) {
			break;
		}
		/* Copy received data for verification later */
		CU_ASSERT_FATAL(len == DATA_LEN);
		memcpy(recv_data, data, len);
		sipc_shm_recv_done(reader_ipc);
	}
	CU_PASS("Successfully read a message via SYSV shared memory");

	/* until sipc_shm_recv_done() is called, further reading of
	   data will cause a non-recoverable state */
	/*
	   CU_ASSERT(sipc_recv_data(reader_ipc, &data, &len) < 0);
	 */
	sipc_shm_recv_done(reader_ipc);

	for (i = 0; i < DATA_LEN; i++) {
		if (recv_data[i] != i % 256) {
			CU_FAIL("Sent data does not match received data!");
			break;
		}
	}
	if (i > DATA_LEN) {
		CU_PASS("Sent data matches received data");
	}
}

/*
 * Test blocking and non-blocking mode.
 */
static void do_blocked_child(void)
{
	char *shm_data = sipc_get_data_ptr(writer_ipc);
	if (!shm_data) {
		sipc_error(writer_ipc, "Unable to get pointer to data.\n");
		return;
	}
	sleep(2);
	shm_data[0] = 0x42;
	if (sipc_send_data(writer_ipc, DATA_LEN) < 0) {
		sipc_error(writer_ipc, "Unable to send data\n");
		return;
	}
	sleep(2);
	shm_data[0] = 0x43;
	if (sipc_send_data(writer_ipc, DATA_LEN) < 0) {
		sipc_error(writer_ipc, "Unable to send data\n");
		return;
	}
	shm_data[0] = 0x44;
	if (sipc_send_data(writer_ipc, DATA_LEN) < 0) {
		sipc_error(writer_ipc, "Unable to send data\n");
		return;
	}
	sipc_close(writer_ipc);
}

static void do_blocked_parent(void)
{
	char *data = NULL;
	size_t len = 0, retv, error;

	printf("This should pause for 2 seconds (default read)...\n");
	if (sipc_recv_data(reader_ipc, &data, &len) < 0) {
		CU_FAIL("Error during first read");
	}
	CU_ASSERT(len == DATA_LEN && data[0] == 0x42);
	sipc_shm_recv_done(reader_ipc);

	printf("No blocking here.\n");
	CU_ASSERT(sipc_ioctl(reader_ipc, SIPC_NOBLOCK) == 0);
	retv = sipc_recv_data(reader_ipc, &data, &len);
	error = errno;
	CU_ASSERT(retv == -1);
	CU_ASSERT(data == NULL);
	CU_ASSERT(len == 0);
	CU_ASSERT(error == EAGAIN);

	printf("This should pause for 3 seconds... (non-blocking read)\n");
	sleep(3);
	if (sipc_recv_data(reader_ipc, &data, &len) < 0) {
		CU_FAIL("Error during first read");
	}
	CU_ASSERT(len == DATA_LEN && data[0] == 0x43);
	sipc_shm_recv_done(reader_ipc);

	CU_ASSERT(sipc_ioctl(reader_ipc, -1) < 0);
	CU_ASSERT(sipc_ioctl(reader_ipc, SIPC_BLOCK) == 0);

	printf("This should pause for 1 second... (blocked read)\n");
	if (sipc_recv_data(reader_ipc, &data, &len) < 0) {
		CU_FAIL("Error during first read");
	}
	CU_ASSERT(len == DATA_LEN && data[0] == 0x44);
	sipc_shm_recv_done(reader_ipc);
}

static void send_end_xmit(sipc_t *sipc)
{
	char *data = sipc_get_data_ptr(sipc);
	if (!data) {
		sipc_error(sipc, "Unable to get data pointer.\n");
		return;
	}

	bzero(data, DATA_LEN);
	strncpy(data, DATA_END, DATA_LEN);
	if (sipc_send_data(sipc, DATA_LEN) < 0) {
		sipc_error(sipc, "Unable to send end of transmission message.\n");
		return;
	}
}

/* Read DATA_LEN bytes from a file and determine if
 * the read contents match the provided character string */
static int verify_data(char *filename, char *recv_data)
{
	FILE *ifile = NULL;
	char data[DATA_LEN];
	int rbytes = 0;

	ifile = fopen(filename, "r");
	if (!ifile) {
		fprintf(stderr, "fopen: %s\n", strerror(errno));
		return -1;
	}

	rbytes = fread(data, sizeof(char), DATA_LEN - 1, ifile);
	if (rbytes < 0) {
		fclose(ifile);
		return -1;
	}
	fclose(ifile);

	if (memcmp(data, recv_data, rbytes))
		return 1;

	return -1;
}

static inline int END_XMIT(char *data)
{
	return !memcmp(data, DATA_END, strlen(DATA_END) + 1);
}
