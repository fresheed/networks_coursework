#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "server/primes_server.h"
#include "node/node_threads.h"
#include "general/messages.h"
#include "general/logic.h"

void* node_proc_thread(void* raw_node_ptr){
  node_data* node=(node_data*)raw_node_ptr;
  messages_set* set=&(node->set);
  int id=node->id;
  while (1){
    message* next_msg=lockNextMessage(set, TO_PROCESS); // now in OWNED state
    if (next_msg == NULL){
      printf("Message set is unactive, stopping to process\n");
      break;
    }
    int continue_proc=nodeProcessMessage(next_msg, set);
    printf("Message processed\n");
    if (!continue_proc){
      printf("Shutting down connection...\n");
      //shutdownWr(node->conn.socket_fd);
      stopNodeThreads(node);
    }
  }
  printf("Stopped to process this node\n");
  return NULL;
}

int nodeProcessMessage(message* msg, messages_set* set){
  if (msg->status_type == RESPONSE) {
    message* req=findMessageById(set, msg->response_to);
    if (req==NULL){
      printf("request not found\n");
    } else {
      if (msg->info_type == MAX_INFO){
	//int resp=msg->data_len;
	unsigned long max_prime;
	readNumsFromChars(msg->data, &max_prime, 1);
	printf("Max prime: %ld\n", max_prime);
      } else if (msg->info_type == RECENT_INFO){
	if (!msg->is_ok){
	  printf("Range request failed (probably data is too long)\n");
	} else {
	  long amount;
	  long shift=readNumsFromChars(msg->data, &amount, 1);
	  long recv_nums[200];
	  readNumsFromChars(msg->data+shift, recv_nums, amount);
	  printf("Recent %ld primes: \n", amount);
	  long i;
	  for (i = 0; i<amount; i++) {
	    printf("%ld; ", recv_nums[i]);
	  }
	  printf("\n");
	}
      }
      updateMessageStatus(req, set, EMPTY_SLOT);
    }
    updateMessageStatus(msg, set, EMPTY_SLOT);
  } else { // request
    if (msg->info_type == COMPUTE_INFO){
      long primes[MAX_RANGE_SIZE];
      long bounds[2];
      readNumsFromChars(msg->data, bounds, 2);
      primes_range range;
      memset(range.numbers, 0, MAX_RANGE_SIZE);
      range.lower_bound=bounds[0];
      range.upper_bound=bounds[1];
      computePrimesInRange(&range);
      message resp;
      fillGeneral(&resp, -1);
      createComputeResponse(&resp, -1, msg->internal_id, &range);
      message* put_msg=putMessageInSet(resp, set, TO_SEND, 1);
      printf("Processed\n");
    } else if (msg->info_type == INIT_SHUTDOWN){
      printf("Server asked this node to shutdown...\n");
      updateMessageStatus(msg, set, EMPTY_SLOT);
      return 0;
    }
    updateMessageStatus(msg, set, EMPTY_SLOT);
  }
  return 1;
}



