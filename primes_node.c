#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "primes_server.h"
#include "primes_node.h"
#include "server_threads.h"

node_data node;

int main(){
  const char* hostname="localhost";
  const int port=3451;

  int socket_fd=connectToServer(hostname, port);
  printf("got %d\n", socket_fd);

  if (socket_fd < 0 ){
    printf("Server connection failed \n");
    exit(1);
  }

  if (!initializeCurrentNode(socket_fd)){
    printf("Node initialization failed\n");
    exit(1);
  }

  processUserInput();
  printf("will finalize\n");
  finalizeCurrentNode();  
  return 0;
}

void processUserInput(){
  const int INPUT_MAX=200;
  char user_input[INPUT_MAX];
  while(1){
    printf("\n=>");    
    fgets(user_input, INPUT_MAX, stdin);
    if (strncmp(user_input, "q", 1)==0){
      printf("QUIT\n");
      break;
    }
  }
}

int initializeCurrentNode(int fd){
  node.id=-1; // we don't care for id at node side
  node.socket_fd=fd;

  initMessagesSet(&(node.set));
  
  pthread_create(&(node.send_thread), NULL,
  		 &server_send_thread, (void*)(&node));
  pthread_create(&(node.recv_thread), NULL,
  		 &server_recv_thread, (void*)(&node));
  pthread_create(&(node.proc_thread), NULL,
  		 &node_proc_thread, (void*)(&node));

  printf("threads created\n");

  return 1;
}

void finalizeCurrentNode(){
  printf("started to finalize current node\n");
  int i;

  close(node.socket_fd);

  printf("joining\n");
  printf("join 0\n");
  pthread_join(node.send_thread, NULL);
  printf("join 1\n");
  pthread_join(node.recv_thread, NULL);
  printf("join 2\n");
  pthread_join(node.proc_thread, NULL);
  printf("join 3\n");
  
  finalizeMessagesSet(&(node.set));

  printf("finalized current node\n");
}

