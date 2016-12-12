#include <sys/types.h>
#include "WinSock2.h"
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include "general/win_sockets.h"

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

socket_conn prepareServerSocket(){
#ifdef TCP_TRANSFER
  return prepareTCPServerSocket();
#else
#ifdef UDP_TRANSFER
  return prepareUDPServerSocket();
#endif
#endif
}

socket_conn prepareTCPServerSocket(){
  const int accept_port=3451;
  struct sockaddr_in server_address;
  printf("Initializing server...\n");
  setServerAddressParams(&server_address, accept_port);

  int server_socket_fd=socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket_fd < 0) {
    return err_socket;
  }
  if (bind(server_socket_fd, (struct sockaddr *)&server_address,
	   sizeof(server_address)) < 0){
    return err_socket;
  }

  listen(server_socket_fd, 1);
  socket_conn conn = {.socket_fd=server_socket_fd};

  return conn;
}

socket_conn prepareUDPServerSocket(){
  const int accept_port=3451;
  struct sockaddr_in server_address;
  printf("Initializing server...\n");
  setServerAddressParams(&server_address, accept_port);

  int server_socket_fd=socket(AF_INET, SOCK_DGRAM, 0);
  if (server_socket_fd < 0) {
    perror("Error on socket creation\n");
    return err_socket;
  }
  if (bind(server_socket_fd, (struct sockaddr *)&server_address,
	   sizeof(server_address)) < 0){
    perror("Error on bind\n");
    return err_socket;
  }

  socket_conn conn = {.socket_fd=server_socket_fd};
  return conn;
}


socket_conn acceptTCPClient(socket_conn server_conn){
  struct sockaddr_in client_address;
  int client_address_size=sizeof(client_address);

  printf("Waiting for client to connect at socket %d...\n", server_conn.socket_fd);
  int client_socket_fd=accept(server_conn.socket_fd, (struct sockaddr*)&client_address, &client_address_size);

  socket_conn conn = {.socket_fd=client_socket_fd};

  return conn;
}

socket_conn acceptUDPClient(socket_conn server_conn, struct sockaddr_in client_address){
  printf("Adding client to socket %d...\n", server_conn.socket_fd);

  socket_conn conn = {.socket_fd=server_conn.socket_fd, // same as listen
		      .peer_address=client_address};
    CreatePipe(&(conn.pipe_out_fd), &(conn.pipe_in_fd), NULL, 0);

  return conn;
}

socket_conn connectToServer(char* hostname, int port){
#ifdef TCP_TRANSFER
  return connectToTCPServer(hostname, port);
#else
#ifdef UDP_TRANSFER
  return connectToUDPServer(hostname, port);
#endif
#endif
}

socket_conn connectToTCPServer(char* hostname, int port){
  int node_socket_fd=socket(AF_INET, SOCK_STREAM, 0);
  if (node_socket_fd < 0){
    printf("error opening node socket!\n");
    return err_socket;
  }

  struct hostent* server=gethostbyname(hostname);
  if (server==NULL){
    printf("error resolving hostname!\n");
    return err_socket;
  }

  struct sockaddr_in server_address;
  setServerAddressParamsForNode(&server_address,  server, port);

  int c=connect(node_socket_fd,(struct sockaddr *)&server_address, sizeof(server_address));
  if (c < 0)
    return err_socket;

  socket_conn conn={.socket_fd=node_socket_fd};
  return conn;
}

socket_conn connectToUDPServer(char* hostname, int port){
  int node_socket_fd=socket(AF_INET, SOCK_DGRAM, 0);
  if (node_socket_fd < 0){
    printf("error opening node socket!\n");
    return err_socket;
  }

  struct hostent* server=gethostbyname(hostname);
  if (server==NULL){
    printf("error resolving hostname!\n");
    return err_socket;
  }

  struct sockaddr_in server_address;
  setServerAddressParamsForNode(&server_address,  server, port);

  char reg_string[]="REG";
  sendto(node_socket_fd, reg_string, strlen(reg_string),
	 0, (struct sockaddr*) &server_address, sizeof(struct sockaddr));
  char server_response[20];
  memset(server_response, 0, 20);
  int tmp;
  recvfrom(node_socket_fd, server_response, 20, 0,
	   (struct sockaddr *) &server_address, &tmp);
  if (strcmp(server_response, "CONNECTED")==0){
    socket_conn conn={.socket_fd=node_socket_fd,
		      .peer_address=server_address};

    CreatePipe(&(conn.pipe_out_fd), &(conn.pipe_in_fd), NULL, 0);
//    pipe(pipe_fds);
//    conn.pipe_in_fd=pipe_fds[1]; // 1 is write end
//    conn.pipe_out_fd=pipe_fds[0]; // 0 is read end
    return conn;
  } else {
    return err_socket;
  }
}

void shutdownWr(int socket_fd){
  shutdown(socket_fd, SD_SEND);
}

void shutdownRd(int socket_fd){
  shutdown(socket_fd, SD_RECEIVE);
}

void shutdownRdWr(int socket_fd){
  shutdown(socket_fd, SD_BOTH);
}

void socketClose(int socket_fd){
    closesocket(socket_fd);
}

int addressesAreEqual(struct sockaddr_in addr1, struct sockaddr_in addr2){
  return ((addr1.sin_addr.s_addr==addr2.sin_addr.s_addr)
	  && (addr1.sin_port==addr2.sin_port));

}

void closePipeDescriptor(pipe_handle fd){
  CloseHandle(fd);
}

int readFromPipe(pipe_handle fd, char* buffer, int to_read){
  //return read(fd, buffer, to_read);
    DWORD was_read;
    DWORD to_read_dword=(DWORD)to_read;
    int success=ReadFile(fd, buffer, to_read_dword, &was_read, NULL);
    if (!success){
        return -1;
    } else {
        return (int)was_read;
    }
}

int writeToPipe(pipe_handle fd, char* buffer, int to_write){
  //return write(fd, buffer, to_read);
    DWORD was_wrote;
    DWORD to_write_dword=(DWORD)to_write;
    int success=WriteFile(fd, buffer, to_write_dword, &was_wrote, NULL);
    if (!success){
        return -1;
    } else {
        return (int)was_wrote;
    }
}

int pipeHasData(pipe_handle fd){
    DWORD total_available_bytes;
    PeekNamedPipe(fd, 0, 0, 0, &total_available_bytes, 0);
    return (total_available_bytes>0);
}

void sleepMs(int ms){
  Sleep(ms);
}

void initSocketsRuntime(){
    WSADATA wsa;
    printf("\nInitialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0){
        printf("WSA Init Failed. Error Code : %d\n",WSAGetLastError());
        return;
    }
    printf("WSA Initialised\n");
}

void finalizeSocketsRuntime(){
    WSACleanup();
}


