#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "server/primes_server.h"
#include "server/server_threads.h"
#include "general/messages.h"

void* server_proc_thread(void* raw_node_ptr){
  node_data* node=(node_data*)raw_node_ptr;
  nodes_info* nodes_params=(nodes_info*)node->nodes_params; 
  messages_set* set=&(node->set);
  int id=node->id;
  while (1){
    message* next_msg=lockNextMessage(set, TO_PROCESS); // now in OWNED state        
    if (next_msg == NULL){
      printf("Message set is unactive, stopping to process\n");
      break;
    }
    serverProcessMessage(next_msg, set);
  }
  printf("Stopped to process node %d\n", id);
  return NULL;
}

void serverProcessMessage(message* msg, messages_set* set){
  if (msg->status_type == REQUEST) {
    if (msg->info_type == MAX_INFO) {
      message resp;
      fillGeneral(&resp, -1);
      createMaxResponse(&resp, -1, msg->internal_id, 200); 
      message* put_msg=putMessageInSet(resp, set, TO_SEND, 1);
    } else if (msg->info_type == RANGE_INFO) {
      message resp;
      int const_resp[]={7, 4, 3};
      createRangeResponse(&resp, -1, msg->internal_id, const_resp, 3); 
      message* put_msg=putMessageInSet(resp, set, TO_SEND, 1);            
    }
    updateMessageStatus(msg, set, EMPTY_SLOT);	
  }
}


