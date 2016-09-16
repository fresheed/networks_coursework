#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

void on_error(char *msg){
  printf(msg); 
  printf("\nErrno:  %d\n", errno);
  char buf[300];
  perror(buf);
  printf(perror);
  exit(1);
}

int main(int argc, char *argv[]){
  const int accept_port=3451;
  int server_socket_fd, client_socket_fd;
  
  int client_address_size, num_bytes_processed;
  
  char buffer[256];
  char response[256];
  struct sockaddr_in server_address, client_address;

  printf("Initializing server...\n");

  server_socket_fd=socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket_fd < 0) {
    on_error("Error opening server socket");
  }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family=AF_INET;
  server_address.sin_port=htons(accept_port); // format port num 
  server_address.sin_addr.s_addr=INADDR_ANY;  // find localhost address
  
  if (bind(server_socket_fd, (struct sockaddr *)&server_address,
	   sizeof(server_address)) < 0){
    close(server_socket_fd);
    on_error("bind failed!");
  }
  listen(server_socket_fd, 1); 
  
  client_address_size=sizeof(client_address);
  printf("Waiting for client to connect...\n");

  client_socket_fd=accept(server_socket_fd, (struct sockaddr*)&client_socket_fd, &client_address_size);
  if (client_socket_fd < 0){
    on_error("client accept failed!");
  }
  
  // at this point client is already connected
  do {
    memset(buffer, 0, 256);
    //num_bytes_processed=read(client_socket_fd, buffer, 255);
    num_bytes_processed=recv(client_socket_fd, buffer, 255, 0);
    if (num_bytes_processed < 0) {
      close(server_socket_fd);
      on_error("Error reading the socket");
    }
    printf("Client message: %s\n", buffer);
    //num_bytes_processed=write(client_socket_fd, "Message received\n", 17);
    
    strcpy(response, "Received:");
    strcat(response, buffer);
    //num_bytes_processed=send(client_socket_fd, "Message received\n", 17);
    num_bytes_processed=send(client_socket_fd, response, strlen(response), 0);
    if (num_bytes_processed < 0) {
      close(server_socket_fd);
      on_error("Error sending response");
    }
  } while ((strcmp(buffer, "exit")) && num_bytes_processed);

  close(server_socket_fd);
  return 0;
}

