#!/bin/bash

touch shm_ipc_test
touch mq_ipc_test
touch mq2_ipc_test
./test_ipc
rm -f shm_ipc_test
rm -f mq_ipc_test
rm -f mq2_ipc_test
