#!/bin/bash

touch sipc_mq_test
touch sipc_shm_test

# Run mq example
echo "** Running message queue example **"
./mq_creator
./mq_sender &
./mq_reader 
./mq_destroyer

# Run shm example
echo -e "\n\n** Running shared memory example **"
./shm_creator
./shm_sender &
./shm_reader 
./shm_destroyer

rm -f sipc_*_test
