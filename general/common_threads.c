#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server/primes_server.h"
#include "general/init_sockets.h"
#include "server/server_threads.h"
#include "general/messages.h"
#include "general/common_threads.h"
#include "transfer/net_transfer.h"

void* common_send_thread(void* raw_node_ptr){
  node_data* node=(node_data*)raw_node_ptr;
  socket_conn conn=node->conn;
  messages_set* set=&(node->set);
  int id=node->id;
  while(1){
    message* msg=lockNextMessage(set, TO_SEND); // now in OWNED state
    if (!set->is_active){
      printf("Message set is unactive, stopping to send\n");
      break;
    }
    /* printf("Sending:\n"); */
    /* printMessage(msg); */
    if (!(sendMessageContent(msg, conn))){
      printf("Send to node %d failed", id);
      // markSetInactive(set); - should be done only by recv thread
      break;
    } else {
      char next_status=(msg->status_type == REQUEST) ? WAITS_RESPONSE : EMPTY_SLOT;
      updateMessageStatus(msg, set, next_status);
    }
  }
  printf("Stopped to send to node %d\n", id);
  return NULL;
}



void* common_recv_thread(void* raw_node_ptr){
  node_data* node=(node_data*)raw_node_ptr;
  messages_set* set=&(node->set);
  char buffer[100];
  int id=node->id;
  socket_conn conn=node->conn;
  while (1){
    message msg;
    fillGeneral(&msg, -1);
    int res=recvMessageContent(&msg, conn);
    if (!res){
      printf("Read from node %d failed\n", id);
      markSetInactive(set);
      break;
    } else {
      /* printf("fetched:\n"); */
      /* printMessage(&msg); */
      message* put_msg=putMessageInSet(msg, set, TO_PROCESS, 0);
      /* printf("Into process:\n"); */
      /* printMessage(put_msg); */
      if (!set->is_active){
	printf("Message set is unactive, stopping to receive\n");
	break;
      }
      //updateMessageStatus(put_msg, set, TO_SEND);
    }
  }
  printf("Stopped to receive from node %d\n", id);
  endCommunication(node);
  return NULL;
}


