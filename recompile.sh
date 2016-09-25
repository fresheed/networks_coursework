#! /bin/bash

rm tcp_server_ref
echo Compiling server:
gcc tcp_server_ref.c init_sockets.c logs.c -o tcp_server_ref -pthread

rm tcp_client
echo Compiling client:
gcc tcp_client.c -o tcp_client
