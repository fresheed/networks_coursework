#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "primes_server.h"

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

  while (1){
    const int send_flags=0;
    int num_bytes_send=send(socket_fd, "node", strlen("node"), send_flags);
    if (num_bytes_send < 0) 
      perror("Error writing to socket\n");

    sleep(3);
  }

  //processAdminInput();
  
  finalizeCurrentNode();  
  return 0;
}

int initializeCurrentNode(int fd){
  node.id=-1; // we don't care for id at node side
  node.socket_fd=fd;

  /* initMessagesSet(&(node.set)); */
  
  /* pthread_create(&(node->send_thread), NULL,  */
  /* 		 &node_send_thread, (void*)(node)); */
  /* pthread_create(&(node->recv_thread), NULL,  */
  /* 		 &node_recv_thread, (void*)(node)); */
  return 1;
}

