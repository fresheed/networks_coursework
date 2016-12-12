#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef _INIT_SOCKETS_H
#define _INIT_SOCKETS_H

#ifdef TCP_TRANSFER
typedef struct {
  int socket_fd;
} socket_conn;
#endif
#ifdef UDP_TRANSFER

typedef int pipe_handle;

typedef struct {
  int socket_fd;
  struct sockaddr_in peer_address; // not used in listen thread
  pipe_handle pipe_in_fd; // used in client threads
  pipe_handle pipe_out_fd; // used in client threads
} socket_conn;
#endif

#define err_socket  (socket_conn){.socket_fd = -1};

#endif

void setServerAddressParams(struct sockaddr_in* server_address, int port);
socket_conn prepareServerSocket();
socket_conn prepareTCPServerSocket();
socket_conn prepareUDPServerSocket();

//socket_conn acceptClient(socket_conn server_conn);
socket_conn acceptTCPClient(socket_conn server_conn);
socket_conn acceptUDPClient(socket_conn server_conn, struct sockaddr_in client_address);

void shutdownWr(int fd);
void shutdownRd(int fd);
void shutdownRdWr(int fd);
void socketClose(int fd);
void initSocketsRuntime();
void finalizeSocketsRuntime();


socket_conn connectToServer(char* hostname, int port);
socket_conn connectToTCPServer(char* hostname, int port);
socket_conn connectToUDPServer(char* hostname, int port);

int addressesAreEqual(struct sockaddr_in addr1, struct sockaddr_in addr2);

int readFromPipe(pipe_handle fd, char* buffer, int to_read);
int writeToPipe(pipe_handle fd, char* buffer, int to_read);
void closePipeDescriptor(pipe_handle fd);

int pipeHasData(pipe_handle fd);
