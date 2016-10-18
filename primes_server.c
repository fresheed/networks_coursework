#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "primes_server.h"
#include "init_sockets.h"
#include "nodes_processing.h"

#include "nodes_threads.h"

server_data server_params;
#define MAX_NODES 2
node_data nodes[MAX_NODES];
nodes_info nodes_params;

int main(){
  if (!initializeServer()){
    printf("Server initialization failed\n");
    exit(1);
  }

  processAdminInput();
  
  finalizeServer();  
  printf("Server stopped\n");

  return 0;
}

void processAdminInput(){
  const int INPUT_MAX=200;
  char admin_input[INPUT_MAX];
  while(1){
    printf("\n=>");
    
    fgets(admin_input, INPUT_MAX, stdin);
    if (strncmp(admin_input, "q", 1)==0){
      printf("QUIT\n");
      break;
    } else if (strncmp(admin_input, "k ", 2)==0) {
      char id_raw=admin_input[2];
      if (isdigit(id_raw)){
	int id_to_kick=id_raw-'0';
	kickNode(&nodes_params, id_to_kick);
      } else {
	printf("Wrong format. Try k (id 0..9)\n");
      }
    }
  }
}

void* runAcceptNodes(){
  while(1){
    int tmp_fd=acceptClient(server_params.listen_socket_fd);
    if (tmp_fd > 0){
      addNewNode(&nodes_params, tmp_fd);
    } else {
      printf("Accept failed\n");
      break;
    }
  }
  
  finalizeNodes(&nodes_params);

  return NULL;
}

int initializeServer(){
  nodes_params.max_nodes=MAX_NODES;
  nodes_params.nodes=nodes;
  int i;
  for (i=0; i<MAX_NODES; i++){
    nodes[i].id=0;
  }
  nodes_params.unique_id_counter=1;
  pthread_mutex_init(&nodes_params.nodes_mutex, NULL);
  pthread_cond_init(&nodes_params.nodes_refreshed, NULL);

  int listen_fd=prepareServerSocket();
  server_params.listen_socket_fd=listen_fd;
  
  pthread_create(&server_params.accept_thread, NULL, &runAcceptNodes, NULL);
  
  return (listen_fd > 0);
}

void finalizeServer(){
  unsigned int server_socket_fd=server_params.listen_socket_fd;
  printf("Closing server socket %d\n", server_socket_fd);
  shutdown(server_socket_fd, SHUT_RDWR);
  close(server_socket_fd);
  printf("Joining accept thread\n");
  closePendingConnections(&nodes_params);
  pthread_join(server_params.accept_thread, NULL);
  pthread_cond_destroy(&nodes_params.nodes_refreshed);
  pthread_mutex_destroy(&nodes_params.nodes_mutex);
}


