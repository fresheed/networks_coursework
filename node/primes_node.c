#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
//#include <sys/socket.h>
#include "server/primes_server.h"
#include "node/primes_node.h"
#include "general/common_threads.h"
#include "general/messages.h"
#include "node/node_threads.h"

node_data node;

int main(int argc, char* argv[]){
  char* hostname="localhost";
  int port=3451;
  if (argc>1){
    hostname=argv[1];
    if (argc>2){
      sscanf(argv[2], "%d", &port);
    }
  }
  printf("Trying to connect %s:%d\n", hostname, port);

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
    memset(user_input, 0, INPUT_MAX);
    printf("\n=>");    
    fgets(user_input, INPUT_MAX, stdin);
    message msg;
    fillGeneral(&msg, -1);
    if (strncmp(user_input, "q", 1)==0){
      printf("QUIT\n");
      break;
    } else if (strncmp(user_input, "rm", 2)==0){
      // request max prime
      createMaxRequest(&msg, -1);
      message* put_msg=putMessageInSet(msg, &(node.set), TO_SEND, 1);
      printf("sending max prime request...\n");
    } else if (strncmp(user_input, "rr", 2)==0){
      // request range
      int lower, upper;
      sscanf(user_input, "rr %d %d", &lower, &upper);
      createRangeRequest(&msg, -1, lower, upper);
      message* put_msg=putMessageInSet(msg, &(node.set), TO_SEND, 1);
      printf("sending range request for %d .. %d\n", lower, upper);
    } else if (strncmp(user_input, "l ", 2)==0){
      // request last N primes
      int to_show;
      sscanf(user_input, "l %d", &to_show);
      if (to_show > 50){
	printf("Only last 50 primes supported\n");
	continue;
      }
      createRecentRequest(&msg, -1, to_show);
      message* put_msg=putMessageInSet(msg, &(node.set), TO_SEND, 1);
      printf("sending recent %d request\n", to_show);
    } 

  }
}

int initializeCurrentNode(int fd){
  node.id=-1; // we don't care for id at node side
  node.socket_fd=fd;

  initMessagesSet(&(node.set));
  
  pthread_create(&(node.send_thread), NULL,
  		 &common_send_thread, (void*)(&node));
  pthread_create(&(node.proc_thread), NULL,
  		 &node_proc_thread, (void*)(&node));
  pthread_create(&(node.recv_thread), NULL,
  		 &common_recv_thread, (void*)(&node));

  printf("threads created\n");

  return 1;
}

void finalizeCurrentNode(){
  printf("started to finalize current node\n");
  int i;

  //close(node.socket_fd);

  printf("joining\n");
  //shutdown(node.socket_fd, SHUT_WR);
  shutdownWr(node.socket_fd);

  pthread_join(node.recv_thread, NULL);
  /* pthread_join(node.send_thread, NULL); */
  /* pthread_join(node.proc_thread, NULL); */
  close(node.socket_fd);
  
  finalizeMessagesSet(&(node.set));

  printf("finalized current node\n");
}

