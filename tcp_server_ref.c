#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

char errbuf[300];
void on_error(char *msg){
  printf(msg); 
  printf("\nErrno:  %d\n", errno);
  perror(errbuf);
  printf(errbuf);
  exit(1);
}

void closeFdAbnormally(int fd, char* msg){
  close(fd);
  on_error(msg);
}


void setServerAddressParams(struct sockaddr_in* server_address, int port){
  memset(server_address, 0, sizeof(*server_address));
  server_address->sin_family=AF_INET;
  server_address->sin_port=htons(port); // format port num 
  server_address->sin_addr.s_addr=INADDR_ANY;  // find localhost address
}

int prepareServerSocket(){
  const int accept_port=3451;
  struct sockaddr_in server_address;
  printf("Initializing server...\n");
  setServerAddressParams(&server_address, accept_port);  

  int server_socket_fd=socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket_fd < 0) {
    on_error("Error opening server socket");
  }
  if (bind(server_socket_fd, (struct sockaddr *)&server_address,
	   sizeof(server_address)) < 0){
    closeFdAbnormally(server_socket_fd, "bind failed!");
  }

  listen(server_socket_fd, 1); 

  return server_socket_fd;
}

int getClientSocket(int server_socket_fd){
  struct sockaddr_in client_address;
  int client_address_size=sizeof(client_address);
  printf("Waiting for client to connect...\n");

  int client_socket_fd=accept(server_socket_fd, (struct sockaddr*)&client_socket_fd, &client_address_size);
  if (client_socket_fd < 0){
    on_error("client accept failed!");
  }

  return client_socket_fd;
}

int readN(int socket_fd, char* read_buf){
  const int Nread=4;
  const int recv_flags=0;
  memset(read_buf, 0, Nread);
  int actual_read=recv(socket_fd, read_buf, Nread, recv_flags);
  printf("Read: %d\n", actual_read);
  return actual_read==Nread;
}

int sendResponse(char* msg, int client_socket_fd){
  static char response[20];
  strcpy(response, "Received:");
  strcat(response, msg);
  int num_bytes_processed=send(client_socket_fd, response, strlen(response), 0);
  return (num_bytes_processed>=0);
}

int main(int argc, char *argv[]){
  char buffer[256];

  int server_socket_fd=prepareServerSocket();
  
  int client_socket_fd=getClientSocket(server_socket_fd);
  
  // at this point client is already connected
  int num_bytes_processed;
  do {
    if (!readN(client_socket_fd, buffer)){
      closeFdAbnormally(server_socket_fd, "Client closed connection or sent less than N bytes\n");
    }
    printf("Client message: %s\n", buffer);
    
    if (!sendResponse(buffer, client_socket_fd)) {
      closeFdAbnormally(server_socket_fd, "Error sending response");
    }
  } while ((strcmp(buffer, "exit")) && num_bytes_processed);

  close(server_socket_fd);
  return 0;
}

