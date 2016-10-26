#! /bin/bash

rm primes_server
echo Compiling server:
gcc primes_server.c nodes_processing.c server_threads.c messages.c init_sockets.c -o primes_server -pthread

rm primes_node
echo Compiling node:
gcc primes_node.c init_sockets.c messages.c server_threads.c -o primes_node -pthread
