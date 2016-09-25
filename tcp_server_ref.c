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

void threadLog(){
  pthread_t this_thread=pthread_self();
  unsigned int tid=(unsigned int)this_thread;
  printf("Thread %d:", tid);
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

int readN(int socket_fd, char* read_buf){
  const int message_len=4;
  char tmp_buf[message_len];
  const int recv_flags=0;
  int total_read=0;

  memset(read_buf, 0, message_len);
  int read_status=0;
  while (total_read < message_len){
    memset(tmp_buf, 0, message_len);
    int to_read_now=message_len-total_read;
    int actual_read_now=recv(socket_fd, tmp_buf, to_read_now, recv_flags);
    printf("Read fragment: %s\n", tmp_buf);
    if (actual_read_now < 0){
      threadLog("Error on reading message data, exiting...");
      read_status=1;
      break;
    }
    total_read+=actual_read_now;
    strcat(read_buf, tmp_buf);
  }
  return read_status==0;
}

int sendResponse(char* msg, int client_socket_fd){
  static char response[20];
  strcpy(response, "Received:");
  strcat(response, msg);
  int num_bytes_processed=send(client_socket_fd, response, strlen(response), 0);
  return (num_bytes_processed>=0);
}

void* processClient(void* arg){
  // at this point client is already connected
  int client_socket_fd=*(int*)arg;
  char buffer[256];
  do {
    if (!readN(client_socket_fd, buffer)){
      printf( "Client closed connection or sent less than N bytes\n");
      break;
    }
    threadLog();
    printf("  Client %d message: %s\n",client_socket_fd,  buffer);
    
    if (!sendResponse(buffer, client_socket_fd)) {
      printf( "Client closed connection or accepted less than N bytes\n");
      break;
    }
  } while ((strcmp(buffer, "exit")));// && num_bytes_processed);
}

#define MAX_CLIENTS 3
void runAcceptClients(int server_socket_fd){
  int clients_fds[MAX_CLIENTS];
  pthread_t clients_threads[MAX_CLIENTS];
  int cl=0;
  while(cl<MAX_CLIENTS){
    int tmp_fd=acceptClient(server_socket_fd);
    if (tmp_fd > 0){
      clients_fds[cl]=tmp_fd;
      pthread_create(&(clients_threads[cl]), NULL, &processClient, &(clients_fds[cl]));
      threadLog();
      printf("Created client thread\n");
      cl++;
    } else {
      threadLog();
      printf("Accept failed\n");
      break;
    }
  }
  
  // after max_clients this func goes here immediately and finished threads
  // last thread could even not send anything
  threadLog();
  printf("Closing client sockets...\n");
  int i;
  for (i=0; i<cl; i++){
    close(clients_fds[i]);
  }
  threadLog();
  printf("Joining client threads...\n");
  for (i=0; i<cl; i++){
    pthread_join(clients_threads[i], NULL);
    printf("Joined and closed client\n");
  }
  
  
    //}
}

int main(int argc, char *argv[]){

  int server_socket_fd=prepareServerSocket();
  
  pthread_t accept_thread;
  pthread_create(&accept_thread, NULL, &runAcceptClients, server_socket_fd);

  char admin_input[100];
  while (1){
    printf("\n=>");
    scanf("%s", admin_input);
    if (strcmp(admin_input, "q")==0){
      printf("QUIT\n");
      break;
    }
  }
  
  printf("Closing server socket %d\n", server_socket_fd);
  shutdown(server_socket_fd, SHUT_RDWR);
  int rescode=close(server_socket_fd);
  printf("close ret code: %d\n", rescode);
  printf("Joining accept thread\n");
  pthread_join(accept_thread, NULL);
  printf("Server stopped\n");

  return 0;
}

