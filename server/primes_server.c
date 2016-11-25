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
#include "transfer/net_transfer.h"

server_data server_params;
#define MAX_NODES 5
node_data nodes[MAX_NODES];
nodes_info nodes_params;
primes_pool pool;

int main(){
  initSocketsRuntime();
  if (!initializeServer()){
    perror("Server initialization failed\n");
    return 1;
  }
  initPool(&pool);

  printf("Range restrict: %d to %ld, max range size is %ld\n", 
	 0, (long)MAX_NUM, (long)MAX_RANGE_SIZE);
  processAdminInput();

  finalizeServer(&server_params, &nodes_params, &pool);
  finalizeSocketsRuntime();
  printf("Server stopped\n");

  return 0;
}

void processAdminInput(){
  const int INPUT_MAX=200;
  char admin_input[INPUT_MAX];
  long test_lower, test_upper;
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
    } else if (strncmp(admin_input, "stf", 3)==0) {
      printNodes(&nodes_params);
      printPoolStatus(&pool, 1);
    } else if (strncmp(admin_input, "sts", 3)==0) {
      printNodes(&nodes_params);
      printPoolStatus(&pool, 0);
    } else if (strncmp(admin_input, "cr1", 3)==0) {
      test_lower=200000000;
      test_upper=200200000;
      int used_executor=assignTaskToNextNode(server_params.last_executor,
					     test_lower, test_upper,
					     &nodes_params);
      if (used_executor < 0){
	printf("No executor available now, please try later\n");
      } else {
	printf("Assigned task to node %d\n", used_executor);
	server_params.last_executor=used_executor;
      }
    } else if (strncmp(admin_input, "cr2", 3)==0) {
      test_lower=250000000;
      test_upper=250200000;
      int used_executor=assignTaskToNextNode(server_params.last_executor,
					     test_lower, test_upper,
					     &nodes_params);
      if (used_executor < 0){
	printf("No executor available now, please try later\n");
      } else {
	printf("Assigned task to node %d\n", used_executor);
	server_params.last_executor=used_executor;
      }
    } else if (strncmp(admin_input, "cr", 2)==0) {
      long lower, upper;
      sscanf(admin_input, "cr %ld %ld", &lower, &upper);
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
    socket_conn new_conn=acceptClient(server_params.listen_conn);
    if (new_conn.socket_fd > 0){
      cleanupZombieNodes(&nodes_params);
      addNewNode(&nodes_params, new_conn, &pool);
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
  nodes_params.pending_conn=err_socket;

  createMutex(&nodes_params.nodes_mutex);
  createCondition(&nodes_params.nodes_refreshed);

  socket_conn listen_conn=prepareTCPServerSocket();
  if (listen_conn.socket_fd < 0){
    return 0;
  }
  server_params.listen_conn=listen_conn;

  runThread(&server_params.accept_thread, &runAcceptNodes, NULL);

  server_params.last_executor=0;

  return 1;
}




