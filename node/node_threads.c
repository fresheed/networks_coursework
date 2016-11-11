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
    printf("Processing...\n");
    message* next_msg=lockNextMessage(set, TO_PROCESS); // now in OWNED state
    printf("will process msg\n");
    if (next_msg == NULL){
      printf("Message set is unactive, stopping to process\n");
      break;
    }
    int continue_proc=nodeProcessMessage(next_msg, set);
    printf("Message processed\n");
    if (!continue_proc){
      printf("Shutting down connection...\n");
      shutdownWr(node->socket_fd);
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
  } else { // request
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
    } else if (msg->info_type == INIT_SHUTDOWN){
      printf("Server asked this node to shutdown...\n");
      return 0;
    }
  }
  return 1;
}



