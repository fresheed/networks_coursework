#! /bin/bash

rm tcp_server
echo Compiling server:
gcc tcp_server.c init_sockets.c logs.c -o tcp_server -pthread

rm tcp_client
echo Compiling client:
gcc tcp_client.c -o tcp_client
