#! /bin/bash

rm tcp_server_ref
echo Compiling server:
gcc tcp_server_ref.c -o tcp_server_ref -pthread

echo Compiling client:
gcc tcp_client.c -o tcp_client
