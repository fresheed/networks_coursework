#! /bin/bash

rm primes_server
echo Compiling server:
gcc primes_server.c init_sockets.c -o primes_server -pthread

# rm tcp_client
# echo Compiling client:
# gcc tcp_client.c -o tcp_client
