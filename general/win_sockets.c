#include <sys/types.h>
#include "WinSock2.h"
#include <string.h>
#include <stdio.h>
#include <windows.h>

void libraryInit(){
	WSADATA WsaData;
	int err = WSAStartup (0x0101, &WsaData);
	if (err == SOCKET_ERROR)	{
           printf ("WSAStartup() failed: %ld\n", GetLastError ());
           exit(1);
	}
}

void setServerAddressParams(struct sockaddr_in* server_address, int port){
  memset(server_address, 0, sizeof(*server_address));
  server_address->sin_family=AF_INET;
  server_address->sin_port=htons(port); // format port num
  server_address->sin_addr.s_addr=INADDR_ANY;  // find localhost address
}

void setServerAddressParamsForNode(struct sockaddr_in* server_address,  struct hostent* server, int port){
  //bzero((char *) &server_address, sizeof(server_address));
  memset(server_address, 0, sizeof(*server_address));
  server_address->sin_family=AF_INET;
  server_address->sin_port=htons(port);
  // h_addr -> (char*)&server_address.sin_addr.s_addr
  /* bcopy(server->h_addr, (char*)&server_address.sin_addr.s_addr, */
  /* 	server->h_length); */

  memcpy((char*)&(server_address->sin_addr.s_addr), server->h_addr, server->h_length);

}

int prepareServerSocket(){
  const int accept_port=3451;
  struct sockaddr_in server_address;
  printf("Initializing server...\n");
  setServerAddressParams(&server_address, accept_port);

  int server_socket_fd=socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket_fd < 0) {
    return -1;
  }
  if (bind(server_socket_fd, (struct sockaddr *)&server_address,
	   sizeof(server_address)) < 0){
    return -1;
  }

  listen(server_socket_fd, 1);

  return server_socket_fd;
}


int acceptClient(int server_socket_fd){
  struct sockaddr_in client_address;
  int client_address_size=sizeof(client_address);

  printf("Waiting for client to connect at socket %d...\n", server_socket_fd);
  int client_socket_fd=accept(server_socket_fd, (struct sockaddr*)&client_address, &client_address_size);

  return client_socket_fd;
}

int connectToServer(char* hostname, int port){
libraryInit();
  int node_socket_fd=socket(AF_INET, SOCK_STREAM, 0);
  if (node_socket_fd < 0){
    printf("error opening node socket!\n");
    return -1;
  }

  struct hostent* server=gethostbyname(hostname);
  if (server==NULL){
    printf("error resolving hostname!\n");
    return -1;
  }

  struct sockaddr_in server_address;
  setServerAddressParamsForNode(&server_address,  server, port);

  int c=connect(node_socket_fd,(struct sockaddr *)&server_address, sizeof(server_address));
  if (c < 0)
    return -1;
  return  node_socket_fd;
}

void shutdownWr(int socket_fd){
  shutdown(socket_fd, SD_SEND);
}

void shutdownRdWr(int socket_fd){
  shutdown(socket_fd, SD_BOTH);
}


