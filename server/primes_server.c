#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
//#include <pthread.h>
#include "server/primes_server.h"
#include "general/init_sockets.h"
#include "general/common_threads.h"
#include "server/nodes_processing.h"
#include "general/logic.h"
#include "server_threads.h"

server_data server_params;
#define MAX_NODES 2
node_data nodes[MAX_NODES];
nodes_info nodes_params;
primes_pool pool;

int main(){
  if (!initializeServer()){
    perror("Server initialization failed\n");
    return 1;
  }
  initPool(&pool);

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
    cleanupZombieNodes(&nodes_params);
    if (strncmp(admin_input, "q", 1)==0){
      printf("QUIT\n");
      break;
    } else if (strncmp(admin_input, "k ", 2)==0) {
      int id_to_kick;
      if (!sscanf(admin_input, "k %d", &id_to_kick)){
	printf("Correct format: k (id>0)\n");
	continue;
      } else {
	if (id_to_kick > 0){
	  kickSingleNode(&nodes_params, id_to_kick);	
	} 
      }
    } else if (strncmp(admin_input, "st", 2)==0) {
      printNodes(&nodes_params);
      printPoolStatus(&pool);
    } else if (strncmp(admin_input, "cr", 2)==0) {
      int lower, upper;
      sscanf(admin_input, "cr %d %d", &lower, &upper);
      if (!validateRangeParams(lower, upper)){
	printf("Given range cannot be computed\n");
	continue;
      }
      int used_executor=assignTaskToNextNode(server_params.last_executor,
					     lower, upper, &nodes_params);
      if (used_executor < 0){
	printf("No executor available now, please try later\n");
      } else {
	printf("Assigned task to node %d\n", used_executor);
	server_params.last_executor=used_executor;
      }
    }
  }
}

void* runAcceptNodes(){
  while(1){
    int tmp_fd=acceptClient(server_params.listen_socket_fd);
    if (tmp_fd > 0){
      cleanupZombieNodes(&nodes_params);
      addNewNode(&nodes_params, tmp_fd, &pool);
    } else {
      printf("Accept failed\n");
      break;
    }
  }

  finalizeNodes(&nodes_params);
  // now our queue is empty
  // so we can init pending connection
  // and close it right now - which is done next

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
  nodes_params.pending_socket=-1;

  createMutex(&nodes_params.nodes_mutex);
  createCondition(&nodes_params.nodes_refreshed);

  int listen_fd=prepareServerSocket();
  if (listen_fd < 0){
    return 0;
  }
  server_params.listen_socket_fd=listen_fd;

  runThread(&server_params.accept_thread, &runAcceptNodes, NULL);

  server_params.last_executor=0;

  return 1;
}

void finalizeServer(){
  unsigned int server_socket_fd=server_params.listen_socket_fd;
  printf("Shutting down server socket %d\n", server_socket_fd);
  // need to do this to unblock server
  closePendingConnections(&nodes_params, &pool);
  //shutdown(server_socket_fd, SHUT_RDWR);
  shutdownRdWr(server_socket_fd);

  printf("Joining accept thread\n");
  waitForThread(&server_params.accept_thread);
  
  destroyCondition(&nodes_params.nodes_refreshed);

  printf("Closing accept socket\n");
  close(server_socket_fd);

  destroyPool(&pool);

  destroyMutex(&nodes_params.nodes_mutex);
}



