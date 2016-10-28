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
  nodes_info* nodes_params=(nodes_info*)node->nodes_params;
  messages_set* set=&(node->set);
  int id=node->id;
  while (1){
    message* next_msg=lockNextMessage(set, TO_PROCESS); // now in OWNED state
    if (next_msg == NULL){
      printf("Message set is unactive, stopping to process\n");
      break;
    }
    nodeProcessMessage(next_msg, set);
  }
  printf("Stopped to process node %d\n", id);
  return NULL;
}

void nodeProcessMessage(message* msg, messages_set* set){
  if (msg->status_type == RESPONSE) {
    message* req=findMessageById(set, msg->response_to);
    if (req==NULL){
      printf("request not found\n");
    } else {
      if (msg->info_type == MAX_INFO){
	//int resp=msg->data_len;
	int max_prime;
	readNumsFromChars(msg->data, &max_prime, 1);
	printf("Max prime: %d\n", max_prime);
      } else if (msg->info_type == RANGE_INFO){
	int amount;
	int shift=readNumsFromChars(msg->data, &amount, 1);
	int recv_nums[200];
	readNumsFromChars(msg->data+shift, recv_nums, amount);
	printf("N primes: %d\n", amount);
	int i;
	for (i = 0; i<amount; i++) {
	  printf("%d; ", recv_nums[i]);
	}
	printf("\n");
      } else if (msg->info_type == RECENT_INFO){
	int amount;
	int shift=readNumsFromChars(msg->data, &amount, 1);
	int recv_nums[200];
	readNumsFromChars(msg->data+shift, recv_nums, amount);
	printf("Recent %d primes: \n", amount);
	int i;
	for (i = 0; i<amount; i++) {
	  printf("%d; ", recv_nums[i]);
	}
	printf("\n");
      }
      updateMessageStatus(req, set, EMPTY_SLOT);
    }
    updateMessageStatus(msg, set, EMPTY_SLOT);
  } else {
    if (msg->info_type == COMPUTE_INFO){
        printf("Processing\n");
      int primes[MAX_RANGE_SIZE];
      int bounds[2];
      readNumsFromChars(msg->data, bounds, 2);
      primes_range range;
      memset(range.numbers, 0, MAX_RANGE_SIZE);
      range.lower_bound=bounds[0];
      range.upper_bound=bounds[1];
      //printf("%d to %d\n", range.lower_bound, range.upper_bound);
      computePrimesInRange(&range);
      message resp;
      fillGeneral(&resp, -1);
      createComputeResponse(&resp, -1, msg->internal_id, range);
      message* put_msg=putMessageInSet(resp, set, TO_SEND, 1);
        printf("Processed\n");
    }
  }
}



