#! /bin/bash

echo Compiling server:
gcc tcp_server_ref.c -o tcp_server_ref

echo Compiling client:
gcc tcp_client.c -o tcp_client
