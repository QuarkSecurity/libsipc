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
#include "mqueue.h"

#define SIPC_MQ_KEY "mq_ipc_test"
#define SIPC_BIG_MQ_KEY "mq2_ipc_test"

#define CREATOR_APP "ipc_creator"
#define DESTROYER_APP "ipc_destroyer"
#define IN_FILE "data"
#define DATA_LEN 8192
#define BIG_DATA_LEN (1024 * 32)
#define XMIT_END "0xDEADBEEF"

static sipc_t *writer_ipc, *reader_ipc;
static sipc_t *writer2_ipc, *reader2_ipc;
static int send_end_xmit(sipc_t *ipc);
static void do_parent(void);
static void do_child(void);
static void do_parent_chunked(void);
static void do_child_chunked(void);
static void do_blocked_parent(void);
static void do_blocked_child(void);
static int verify_data(char *filename, char *recv_data);
static inline int is_end_xmit(char *data);

int test_mqueue_init(void)
{
	pid_t pid;
	int status = 0;
	char ipc_type_str[128];

	bzero(ipc_type_str, 128);
	snprintf(ipc_type_str, 128, "%d", SIPC_SYSV_MQUEUES);
	char *helper_argv[] = {
		CREATOR_APP,
		SIPC_MQ_KEY,
		ipc_type_str,
		NULL,
	};

	/* Run helper app first */
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
	helper_argv[1] = SIPC_BIG_MQ_KEY;
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
	writer_ipc = sipc_open(SIPC_MQ_KEY, SIPC_SENDER, SIPC_SYSV_MQUEUES, DATA_LEN);
	if (writer_ipc == NULL) {
		fprintf(stderr, "Unable to initialize IPC resource\n");
		return -1;
	}
	reader_ipc = sipc_open(SIPC_MQ_KEY, SIPC_RECEIVER, SIPC_SYSV_MQUEUES, DATA_LEN);
	if (reader_ipc == NULL) {
		fprintf(stderr, "Error initializing IPC resource\n");
		return -1;
	}

	writer2_ipc = sipc_open(SIPC_BIG_MQ_KEY, SIPC_SENDER, SIPC_SYSV_MQUEUES, BIG_DATA_LEN);
	if (writer2_ipc == NULL) {
		fprintf(stderr, "Unable to initialize big IPC resource\n");
		return -1;
	}
	reader2_ipc = sipc_open(SIPC_BIG_MQ_KEY, SIPC_RECEIVER, SIPC_SYSV_MQUEUES, BIG_DATA_LEN);
	if (reader2_ipc == NULL) {
		fprintf(stderr, "Error initializing big IPC resource\n");
		return -1;
	}
	return 0;
}

int test_mqueue_cleanup(void)
{
	int status;
	pid_t pid;
	char ipc_type_str[128];

	/* Cleanup handles here */
	sipc_close(reader_ipc);
	sipc_close(writer_ipc);
	sipc_close(reader2_ipc);
	sipc_close(writer2_ipc);

	/* Build the destroyer app's argv */
	bzero(ipc_type_str, 128);
	snprintf(ipc_type_str, 128, "%d", SIPC_SYSV_MQUEUES);
	char *helper_argv[] = {
		DESTROYER_APP,
		SIPC_MQ_KEY,
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

	helper_argv[1] = SIPC_BIG_MQ_KEY;
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

static void test_unforked_mqueue(void)
{
	static char message[] = "HELLO,";
	size_t message_len = sizeof(message);
	char *data = sipc_get_data_ptr(writer_ipc);
	size_t recv_len;
	char *recv_data;

	CU_ASSERT_PTR_NOT_NULL_FATAL(data);
	strcpy(data, message);
	CU_ASSERT(sipc_send_data(writer_ipc, message_len) == 0);
	CU_ASSERT(sipc_recv_data(reader_ipc, &recv_data, &recv_len) == 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(recv_data);
	CU_ASSERT(recv_len >= message_len);
	CU_ASSERT(memcmp(recv_data, message, message_len) == 0);
	free(recv_data);
}

static void test_mqueue(void)
{
	pid_t pid;
	int status = 0;

	switch ((pid = fork())) {
	case -1:
		fprintf(stderr, "fork: %s\n", strerror(errno));
		return;
	case 0:
		sipc_close(reader_ipc);
		do_child();
		exit(0);
		break;
	default:
		do_parent();
		break;
	}

	wait(&status);
}

static void test_chunked_mqueue(void)
{
	pid_t pid;
	int status = 0;

	switch ((pid = fork())) {
	case -1:
		fprintf(stderr, "fork: %s\n", strerror(errno));
		return;
	case 0:
		sipc_close(reader2_ipc);
		do_child_chunked();
		exit(0);
		break;
	default:
		do_parent_chunked();
		break;
	}

	wait(&status);
}

static void test_blocked_mqueue(void)
{
	pid_t pid;
	int status = 0;

	switch ((pid = fork())) {
	case -1:
		fprintf(stderr, "fork: %s\n", strerror(errno));
		return;
	case 0:
		sipc_close(reader2_ipc);
		do_blocked_child();
		exit(0);
		break;
	default:
		do_blocked_parent();
		break;
	}

	wait(&status);
}

CU_TestInfo mqueue_tests[] = {
	{"unforked mqueue", test_unforked_mqueue}
	,
	{"forked mqueue", test_mqueue}
	,
	{"forked chunked mqueue", test_chunked_mqueue}
	,
	{"forked blocked mqueue", test_blocked_mqueue}
	,
	CU_TEST_INFO_NULL
};

/* Read a message from mqueue */
static void do_parent(void)
{
	char *data = NULL;
	size_t len = 0;

	while (!sipc_recv_data(reader_ipc, &data, &len)) {
		CU_ASSERT_PTR_NOT_NULL_FATAL(data);
		if (is_end_xmit(data)) {
			free(data);
			break;
		}
		if (verify_data(IN_FILE, data) != 1) {
			CU_PASS("Sent data matches received data");
		} else {
			CU_FAIL("Sent data does not match received data!");
		}
		free(data);
	}

	CU_ASSERT(sipc_recv_data(reader_ipc, &data, &len) == 0);
	CU_ASSERT_PTR_NOT_NULL(data);
	CU_ASSERT(len == 0);
	free(data);
}

static void do_child(void)
{
	int rbytes = 0;
	char *data = NULL;
	FILE *ifile = fopen(IN_FILE, "r");
	if (!ifile) {
		fprintf(stderr, "fopen: %s\n", strerror(errno));
		return;
	}

	/* Get pointer to IPC's data member */
	data = sipc_get_data_ptr(writer_ipc);
	CU_ASSERT_PTR_NOT_NULL(data);

	/* Read DATA_LEN bytes from file */
	rbytes = fread(data, sizeof(char), DATA_LEN, ifile);
	if (rbytes < 0) {
		fprintf(stderr, "fread: %s\n", strerror(errno));
		fclose(ifile);
		return;
	}

	/* Send data, then end of xmit */
	CU_ASSERT(sipc_send_data(writer_ipc, DATA_LEN) == 0);
	CU_ASSERT(send_end_xmit(writer_ipc) == 0);
	fclose(ifile);

	/* Trying sending a zero-byte message */
	CU_ASSERT(sipc_send_data(writer_ipc, 0) == 0);

	/* Cleanup handle here */
	sipc_close(writer_ipc);
}

static void do_parent_chunked(void)
{
	char *data = NULL;
	size_t len = 0;

	size_t j = 0;
	CU_ASSERT(sipc_recv_data(reader2_ipc, &data, &len) == 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(data);
	CU_ASSERT_FATAL(len == BIG_DATA_LEN);

	size_t i;
	for (i = 0; i < len; i++) {
		if (i % 512 == 0) {
			j++;
		}
		CU_ASSERT(data[i] == j);
	}
	free(data);

	CU_ASSERT(sipc_recv_data(reader2_ipc, &data, &len) == 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(data);
	CU_ASSERT_FATAL(len == BIG_DATA_LEN - 1);

	j = 42;
	for (i = 0; i < len; i++) {
		CU_ASSERT(data[i] == j);
		j++;
		if (i % 120 == 0) {
			j = 0;
		}
	}
	free(data);
}

static void do_child_chunked(void)
{
	char *data = NULL;

	/* Get pointer to IPC's data member */
	data = sipc_get_data_ptr(writer2_ipc);
	CU_ASSERT_PTR_NOT_NULL(data);

	size_t i, j = 0;
	for (i = 0; i < BIG_DATA_LEN; i++) {
		if (i % 512 == 0) {
			j++;
		}
		data[i] = j;
	}

	/* Send data, then end of xmit */
	CU_ASSERT(sipc_send_data(writer2_ipc, i) == 0);

	/* This time send data that does not nicely fit on a word boundary */
	j = 42;
	for (i = 0; i < BIG_DATA_LEN - 1; i++) {
		data[i] = j++;
		if (i % 120 == 0) {
			j = 0;
		}
	}

	/* Send data, then end of xmit */
	CU_ASSERT(sipc_send_data(writer2_ipc, i) == 0);

	/* Cleanup handle here */
	sipc_close(writer2_ipc);
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
	if (sipc_send_data(writer_ipc, 1) < 0) {
		sipc_error(writer_ipc, "Unable to send data\n");
		return;
	}
	sleep(2);
	shm_data[0] = 0x43;
	if (sipc_send_data(writer_ipc, 1) < 0) {
		sipc_error(writer_ipc, "Unable to send data\n");
		return;
	}
	shm_data[0] = 0x44;
	if (sipc_send_data(writer_ipc, 1) < 0) {
		sipc_error(writer_ipc, "Unable to send data\n");
		return;
	}
	sipc_close(writer_ipc);
}

static void do_blocked_parent(void)
{
	char *data = NULL;
	size_t len = 0;
	int retv, error;

	printf("This should pause for 2 seconds (default read)...\n");
	if (sipc_recv_data(reader_ipc, &data, &len) < 0) {
		CU_FAIL("Error during first read");
	}
	CU_ASSERT(len == 1 && data[0] == 0x42);
	free(data);

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
	CU_ASSERT(len == 1 && data[0] == 0x43);
	free(data);

	CU_ASSERT(sipc_ioctl(reader_ipc, -1) < 0);
	CU_ASSERT(sipc_ioctl(reader_ipc, SIPC_BLOCK) == 0);

	printf("This should pause for 1 second... (blocked read)\n");
	if (sipc_recv_data(reader_ipc, &data, &len) < 0) {
		CU_FAIL("Error during first read");
	}
	CU_ASSERT(len == 1 && data[0] == 0x44);
	free(data);
}

static int send_end_xmit(sipc_t *ipc)
{
	char *data = sipc_get_data_ptr(ipc);
	if (!data) {
		sipc_error(ipc, "Unable to get internal data pointer from IPC resource\n");
		return -1;
	}

	bzero(data, DATA_LEN);
	strncpy(data, XMIT_END, DATA_LEN - 1);
	if (sipc_send_data(ipc, DATA_LEN) < 0) {
		sipc_error(ipc, "Unable to send end of transmission marker\n");
		return -1;
	}

	return 0;
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

static inline int is_end_xmit(char *data)
{
	return !strcmp(data, XMIT_END);
}
