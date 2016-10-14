#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "primes_server.h"
#include "nodes_threads.h"

void* node_send_thread(void* raw_node_ptr){
  node_data* node=(node_data*)raw_node_ptr; 
  while(1){
    break;
  }
  fflush(stdout);

  return NULL;
}

void* node_recv_thread(void* raw_node_ptr){
  node_data* node=(node_data*)raw_node_ptr;
  char buffer[100];
  while (1){
    if (!readN(node->socket_fd, buffer)){
      printf("Read from node %d failed\n", node->id);
      break;
    } else {
      printf("Message from node %d: %s\n", node->id, buffer);
    }
  }
  printf("Stopped to receive from node %d\n", node->id);
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

