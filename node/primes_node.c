#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "general/init_sockets.h"
#include "server/primes_server.h"
#include "node/primes_node.h"
#include "general/common_threads.h"
#include "general/messages.h"
#include "node/node_threads.h"
#include "transfer/net_transfer.h"

node_data node;

int main(int argc, char* argv[]){
  initSocketsRuntime();
  char* hostname="localhost";
  int port=3451;
  if (argc>1){
    hostname=argv[1];
    if (argc>2){
      sscanf(argv[2], "%d", &port);
    }
  }
  printf("Trying to connect %s:%d\n", hostname, port);

  socket_conn conn=connectToServer(hostname, port);

  if (conn.socket_fd < 0 ){
    printf("Server connection failed \n");
    exit(1);
  }

  if (!initializeCurrentNode(conn)){
    printf("Node initialization failed\n");
    exit(1);
  }

  processUserInput();
  finalizeSocketsRuntime();
  finalizeCurrentNode(&node);
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

int initializeCurrentNode(socket_conn conn){
  node.id=-1; // we don't care for id at node side
  node.conn=conn;

  initMessagesSet(&(node.set));
  runThread(&(node.send_thread), &common_send_thread, (void*)(&node));
  runThread(&(node.proc_thread), &node_proc_thread, (void*)(&node));
  runThread(&(node.recv_thread), &common_recv_thread, (void*)(&node));


  printf("threads created\n");

  return 1;
}
