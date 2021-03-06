#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "server/primes_server.h"
#include "server/server_threads.h"
#include "general/messages.h"

void* server_proc_thread(void* raw_node_ptr){
  node_data* node=(node_data*)raw_node_ptr;
  messages_set* set=&(node->set);
  primes_pool* pool=node->common_pool;
  int id=node->id;
  while (1){
    message* next_msg=lockNextMessage(set, TO_PROCESS); // now in OWNED state
    if (next_msg == NULL){
      printf("Message set is unactive, stopping to process\n");
      break;
    }
    serverProcessMessage(next_msg, set, pool);
  }
  printf("Stopped to process node %d\n", id);
  return NULL;
}

void serverProcessMessage(message* msg, messages_set* set, primes_pool* pool){
  if (msg->status_type == REQUEST) {
    if (msg->info_type == MAX_INFO) {
      message resp;
      fillGeneral(&resp, -1);
      int max=getCurrentMaxPrime(pool);
      createMaxResponse(&resp, -1, msg->internal_id, max);
      message* put_msg=putMessageInSet(resp, set, TO_SEND, 1);
    } else if (msg->info_type == RECENT_INFO) {
      long amount;
      long shift=readNumsFromChars(msg->data, &amount, 1);
      message resp;
      long recent_primes[MAX_RANGE_SIZE];
      getRecentPrimes(amount, pool, recent_primes);
      createRecentResponse(&resp, -1, msg->internal_id, recent_primes, amount);
      message* put_msg=putMessageInSet(resp, set, TO_SEND, 1);
    }
    updateMessageStatus(msg, set, EMPTY_SLOT);
  } else {
    message* req=findMessageById(set, msg->response_to);
    if (req==NULL){
      printf("request not found\n");
      updateMessageStatus(msg, set, EMPTY_SLOT);
    }
    if (msg->info_type == COMPUTE_INFO) {
      if (!msg->is_ok){
	printf("Compute request failed (probably data is too long)\n");
      } else {
	// process response for range computation
	long amount;	
	long shift=readNumsFromChars(msg->data, &amount, 1);
	long bounds[2];
	long shift2=readNumsFromChars(msg->data+shift, bounds, 2);
	long recv_nums[MAX_RANGE_SIZE];
	readNumsFromChars(msg->data+shift+shift2, recv_nums, amount);
	primes_range range;
	range.lower_bound=bounds[0];
	range.upper_bound=bounds[1];
	memset(range.numbers, 0, MAX_RANGE_SIZE);
	setRangeNumbers(&range, recv_nums, amount);
	putRangeInPool(range,  pool);
      }
    }
    updateMessageStatus(msg, set, EMPTY_SLOT);
    updateMessageStatus(req, set, EMPTY_SLOT);
  }
}


