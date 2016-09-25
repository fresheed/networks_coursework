#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>

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
  //listen(server_socket_fd, 0); 

  return server_socket_fd;
}


int acceptClient(int server_socket_fd){
  struct sockaddr_in client_address;
  int client_address_size=sizeof(client_address);
  
  threadLog();
  printf("Waiting for client to connect at socket %d...\n", server_socket_fd);
  int client_socket_fd=accept(server_socket_fd, (struct sockaddr*)&client_address, &client_address_size);

  return client_socket_fd;
}

