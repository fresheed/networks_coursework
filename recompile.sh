#! /bin/bash

rm primes_server
echo Compiling server:
gcc -I . server/primes_server.c server/nodes_processing.c server/server_threads.c general/messages.c general/init_sockets.c general/common_threads.c general/logic.c general/u_threads.c transfer/net_transfer.c -D TCP_TRANSFER -o primes_server -pthread -lm

rm primes_node
echo Compiling node:
gcc -I . node/primes_node.c general/init_sockets.c general/messages.c node/node_threads.c general/common_threads.c general/logic.c  general/u_threads.c server/nodes_processing.c server/server_threads.c transfer/net_transfer.c -D TCP_TRANSFER -o primes_node -pthread -lm
