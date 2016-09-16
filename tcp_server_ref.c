#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>

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

int waitForClientConnection(int server_socket_fd){
  struct sockaddr_in client_address;
  int client_address_size=sizeof(client_address);
  printf("Waiting for client to connect...\n");

  int client_socket_fd=accept(server_socket_fd, (struct sockaddr*)&client_address, &client_address_size);
  if (client_socket_fd > 0){ 
    printf("ACCEPTED at server socket %d\n", server_socket_fd);
  }
  return client_socket_fd;
}

int readN(int socket_fd, char* read_buf){
  const int Nread=4;
  const int recv_flags=0;
  memset(read_buf, 0, Nread);
  int actual_read=recv(socket_fd, read_buf, Nread, recv_flags);
  return actual_read==Nread;
}

int sendResponse(char* msg, int client_socket_fd){
  static char response[20];
  strcpy(response, "Received:");
  strcat(response, msg);
  int num_bytes_processed=send(client_socket_fd, response, strlen(response), 0);
  return (num_bytes_processed>=0);
}

//void processClient(int client_socket_fd, int server_socket_fd){
//void processClient(struct socket_pair sockets_data){
void processClient(int client_socket_fd){
  char buffer[256];
  // at this point client is already connected
  do {
    if (!readN(client_socket_fd, buffer)){
      printf("Client closed connection or sent less than N bytes\n");
      break;
    }
    printf("  Message from client %d: %s\n", client_socket_fd,  buffer);
    
    if (!sendResponse(buffer, client_socket_fd)) {
      printf("  Client closed connection or sent less than N bytes\n");
      break;
    }
  } while ((strcmp(buffer, "exit")));// && num_bytes_processed);
  printf("  Finished client %d\n", client_socket_fd);
}

#define MAX_CLIENTS 2
pthread_t client_threads[MAX_CLIENTS];
int client_fds[MAX_CLIENTS];
void runAcceptClients(int server_socket_fd){
  int clients_accepted=0;
  while (clients_accepted < MAX_CLIENTS){
    printf("Before waitFor... \n");
    int client_socket_fd=waitForClientConnection(server_socket_fd);
    if (client_socket_fd < 0) {
      printf("Cannot accept new socket, exiting\n");
      break;
    } else {
      printf("New connection accepted\n"); 
      client_fds[clients_accepted]=client_socket_fd;
      pthread_create(&client_threads[clients_accepted], NULL,
		     &processClient, client_socket_fd);
      clients_accepted++;
    }
  }
  int i;
  // need 2 cycles, because otherwise it will wait for first thread
  // and in this time other threads will be executing
  for (i=0; i<clients_accepted; i++){
    close(client_fds[i]);
    printf("Closed client %d\n", i);
  }
  for (i=0; i<clients_accepted; i++){
    pthread_join(client_threads[i], NULL);
    printf("Joined client %d\n", i);
  }
}


int main(int argc, char *argv[]){

  int server_socket_fd=prepareServerSocket();
  printf("Created server socket %d\n", server_socket_fd);
  
  pthread_t accept_thread;
  pthread_create(&accept_thread, NULL, &runAcceptClients, server_socket_fd);

  char admin_input[50];
  int tmp=5;
  while (tmp--){
    printf("=> ");
    scanf("%s", admin_input);
    if (!strcmp(admin_input, "quit")){
      printf("Exiting...\n");
      break;
    }
  }


  int is_closed=close(server_socket_fd);
  printf("is closed: %d\n", is_closed);
  printf("closed server socket %d\n", server_socket_fd);
  printf("Server socket closed\n");
  pthread_join(accept_thread, NULL);


  return 0;
}

