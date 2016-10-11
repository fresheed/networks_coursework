#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "primes_server.h"
#include "init_sockets.h"

int main(){
  server_data data;

  const int MAX_NODES=2;
  node_data nodes[MAX_NODES];
  nodes_info nodes_struct;

  if (!initializeServerData(&data) 
      || !initializeNodes(&nodes_struct, MAX_NODES, nodes)){
    printf("Server initialization failed\n");
    exit(1);
  }

  processAdminInput(&data);
  
  finalizeServer(&data);  
  printf("Server stopped\n");

}

void processAdminInput(server_data* data){
  const int INPUT_MAX=200;
  char admin_input[INPUT_MAX];
  while(1){
    printf("\n=>");
    fgets(admin_input, INPUT_MAX, stdin);
    if (strncmp(admin_input, "q", 1)==0){
      printf("QUIT\n");
      break;
    } 
  }
}

void runAcceptNodes(void* raw_data){
  server_data* data=(server_data*)raw_data;
  
  while(1){
    int tmp_fd=acceptClient(server_socket_fd);
    if (tmp_fd > 0){
      addNewNode(data, temp_fd);
    } else {
      printf("Accept failed\n");
      break;
    }
  }
  
  printf("Closing client sockets...\n");
  int i;
  pthread_mutex_lock(&clients_mutex);
  for (i=0; i<clients_count; i++){
    closeNodeThreads(i)
  }
  
  printf("Joining client threads...\n");
  for (i=0; i<clients_count; i++){
    joinNode(i);
  } 
  pthread_mutex_unlock(&clients_mutex);
}

void addNewNode(node_data* nodes, pthread_mutex_t nodes_mutex){
  
}


int initializeServerData(server_data* data){
  int listen_fd=prepareServerSocket();
  data->listen_socket_fd=listen_fd;
  pthread_create(&data->accept_thread, NULL, &runAcceptNodes, data);
  return (listen_fd > 0);
}

int initializeNodes(nodes_info* nodes_struct, int max_nodes, node_data* nodes_array){
  nodes_struct->max_nodes=max_nodes;
  nodes_struct->nodes=nodes_array;
  return 1;
}

void finalizeServer(server_data* data){
  unsigned int server_socket_fd=data->listen_socket_fd;
  printf("Closing server socket %d\n", server_socket_fd);
  shutdown(server_socket_fd, SHUT_RDWR);
  printf("Joining accept thread\n");
  pthread_join(data->accept_thread, NULL);
}


