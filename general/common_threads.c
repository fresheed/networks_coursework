#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server/primes_server.h"
#include "general/init_sockets.h"
#include "server/server_threads.h"
#include "general/messages.h"
#include "general/common_threads.h"

void* common_send_thread(void* raw_node_ptr){
  node_data* node=(node_data*)raw_node_ptr;
  int socket_fd=node->socket_fd;
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
    if (!(sendMessageContent(msg, socket_fd))){
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

int sendMessageContent(message* msg, int socket_fd){
  char buf[200];
  const int send_flags=0;
  int needed_len=5, actual_sent;
  memset(buf, 0, 200);
  buf[0]=msg->internal_id;
  buf[1]=msg->status_type;
  buf[2]=msg->info_type;
  buf[3]=msg->response_to;
  // current_status ignored!
  buf[4]=msg->data_len;
  if (msg->data != NULL){
    memcpy(buf+5, msg->data, msg->data_len);
    needed_len+=msg->data_len;
  }
  if ((msg->data_len == 0) && (msg->data != NULL)){
    printf("Strange msg send:\n");
    printMessage(msg);
  }

  actual_sent=send(socket_fd, buf, needed_len, send_flags);
  return (actual_sent == needed_len);
}

// should be executed from recv thread only
void endCommunication(node_data* node){
  /* pthread_join(node->proc_thread, NULL); */
  /* pthread_join(node->send_thread, NULL); */
  waitForThread(&(node->proc_thread));
  waitForThread(&(node->send_thread));

  // at this point peer should sent shutdown already
  //shutdown(node->socket_fd, SHUT_WR);

  shutdownWr(node->socket_fd);
  close(node->socket_fd);
}


void* common_recv_thread(void* raw_node_ptr){
  node_data* node=(node_data*)raw_node_ptr;
  messages_set* set=&(node->set);
  char buffer[100];
  int id=node->id;
  int socket_fd=node->socket_fd;
  while (1){
    message msg;
    fillGeneral(&msg, -1);
    printf("listening...\n");
    int res=recvMessageContent(&msg, socket_fd);
    printf("recv : %d\n", res);
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

int recvMessageContent(message* msg, int socket_fd){
  char buf[200];
  memset(buf, 0, 200);
  // 1: read header of 5 bytes
  if (!readN(socket_fd, buf, 5)){
    printf("readN failed!\n");
    return 0;
  }
  msg->internal_id=buf[0];
  msg->status_type=buf[1];
  msg->info_type=buf[2];
  msg->response_to=buf[3];
  // current_status ignored!
  msg->data_len=buf[4];
  if (msg->data_len != 0){
    readN(socket_fd, buf+5, msg->data_len);
    addData(msg, buf+5, msg->data_len);
  }
  printMessage(msg);
  if ((msg->data_len == 0) && (msg->data != NULL)){
    printf("Strange msg recv:\n");
    printMessage(msg);
  }
  // 2: read add data ...
  return 1;
}


int readN(int socket_fd, char* read_buf, int message_len){
  //char tmp_buf[message_len];
  char tmp_buf[200];
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
      printf("failed to read anything\n");
      break;
    } else if (actual_read_now == 0){
      printf("peer has shut connection down\n");
      read_status=1;
      break;
    }
    //strcat(read_buf, tmp_buf);
    memcpy( (read_buf+total_read), (tmp_buf), actual_read_now);
    total_read+=actual_read_now;
  }
  int i;
  for (i=0; i<total_read; i++){
    int tmp;
    tmp=read_buf[i];
  }
  return read_status==0;
}
