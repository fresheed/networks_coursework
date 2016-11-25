#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef _INIT_SOCKETS_H
#define _INIT_SOCKETS_H

typedef struct {
  int socket_fd;
  struct sockaddr_in peer_address; // used for udp only
} socket_conn;

#define err_socket  (socket_conn){.socket_fd = -1};

#endif

void setServerAddressParams(struct sockaddr_in* server_address, int port);
socket_conn prepareServerSocket();
socket_conn prepareTCPServerSocket();
socket_conn prepareUDPServerSocket();

socket_conn acceptClient(socket_conn server_conn);
socket_conn acceptTCPClient(socket_conn server_conn);
socket_conn acceptUDPClient(socket_conn server_conn);

void shutdownWr(socket_conn sock);
void shutdownRdWr(socket_conn sock);
void socketClose(socket_conn sock);
void initSocketsRuntime();
void finalizeSocketsRuntime();


socket_conn connectToServer(char* hostname, int port);
socket_conn connectToTCPServer(char* hostname, int port);
socket_conn connectToUDPServer(char* hostname, int port);


