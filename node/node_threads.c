#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "server/primes_server.h"
#include "node/node_threads.h"
#include "general/messages.h"

void* node_proc_thread(void* raw_node_ptr){
  node_data* node=(node_data*)raw_node_ptr;
  nodes_info* nodes_params=(nodes_info*)node->nodes_params; 
  messages_set* set=&(node->set);
  int id=node->id;
  while (1){
    message* msg=lockNextMessage(set, TO_PROCESS); // now in OWNED state        
    if (msg == NULL){
      printf("Message set is unactive, stopping to process\n");
      break;
    }
    message next_msg;
    createRequest(&next_msg, -1, INCOMING);
    message* put_msg=putMessageInSet(next_msg, set, TO_SEND);
    sleep(1);
    updateMessageStatus(msg, set, EMPTY_SLOT);
  }
  printf("Stopped to process node %d\n", id);
  return NULL;
}


