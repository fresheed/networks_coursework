#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "primes_server.h"
#include "server_threads.h"
#include "messages.h"

void* server_send_thread(void* raw_node_ptr){
  node_data* node=(node_data*)raw_node_ptr; 
  nodes_info* nodes_params=(nodes_info*)node->nodes_params; 
  int socket_fd=node->socket_fd;
  messages_set* set=&(node->set);
  const int send_flags=0;
  int id=node->id;
  while(1){
    message* msg=lockNextMessage(set, TO_SEND); // now in OWNED state    
    if (!set->is_active){
      printf("Message set is unactive, stopping to send\n");
      break;
    }
    char* msg_text="cba";
    int msg_len=strlen(msg_text);
    int actual_sent=send(socket_fd, msg_text, msg_len, send_flags);
    if (actual_sent != msg_len){
      printf("Send to node %d failed, sent %d of %d\n", id, actual_sent, msg_len);
      markSetInactive(set);
      break;      
    } else {
      updateMessageStatus(msg, set, EMPTY_SLOT);
      printf("sent message\n");    
    }
  }
  printf("Stopped to send to node %d\n", id);
  return NULL;
}

void* server_recv_thread(void* raw_node_ptr){
  node_data* node=(node_data*)raw_node_ptr;
  nodes_info* nodes_params=(nodes_info*)node->nodes_params; 
  messages_set* set=&(node->set);
  char buffer[100];
  int id=node->id;
  while (1){
    printf("reading\n");
    if (!readN(node->socket_fd, buffer)){
      printf("Read from node %d failed\n", id);
      markSetInactive(set);
      break;
    } else {
      printf("creating REQUEST for message\n");
      message msg;
      createRequest(&msg, -1, INCOMING);
      message* put_msg=putMessageInSet(msg, set, TO_PROCESS);
      if (!set->is_active){
	printf("Message set is unactive, stopping to receive\n");
	break;
      }
      //updateMessageStatus(put_msg, set, TO_SEND);
    }
  }
  printf("Stopped to receive from node %d\n", id);
  return NULL;
}

void* server_proc_thread(void* raw_node_ptr){
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
    updateMessageStatus(msg, set, EMPTY_SLOT);
  }
  printf("Stopped to process node %d\n", id);
  return NULL;
}

int readN(int socket_fd, char* read_buf){
  const int message_len=4;
  char tmp_buf[message_len];
  const int recv_flags=0;
  int total_read=0;

  memset(read_buf, 0, message_len);
  int read_status=0;
  while (total_read < message_len){
    memset(tmp_buf, 0, message_len);
    int to_read_now=message_len-total_read;
    int actual_read_now=recv(socket_fd, tmp_buf, to_read_now, recv_flags);
    if (actual_read_now < 0){
      read_status=1;
      break;
    }
    total_read+=actual_read_now;
    strcat(read_buf, tmp_buf);
  }
  return read_status==0;
}

