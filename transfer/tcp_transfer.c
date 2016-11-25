#include "transfer/tcp_transfer.h"
#include "server/nodes_processing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sendMessageContent(message* msg, int socket_fd){
  char buf[LIMIT_DATA_LEN+HEADER_LEN];
  const int send_flags=0;
  int needed_len=HEADER_LEN, actual_sent;
  memset(buf, 0, LIMIT_DATA_LEN+HEADER_LEN);
  buf[0]=msg->internal_id;
  buf[1]=msg->status_type;
  buf[2]=msg->info_type;
  buf[3]=msg->response_to;
  buf[4]=msg->is_ok;
  sprintf(buf+5, "%06d", msg->data_len);
  // current_status ignored!
  /* buf[4]=msg->data_len; */
  /* buf[5]=msg->data_len; */
  /* buf[6]=msg->data_len; */
  if (msg->data != NULL){
    memcpy(buf+HEADER_LEN, msg->data, msg->data_len);
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
  waitForThread(&(node->proc_thread));
  waitForThread(&(node->send_thread));

  // at this point peer should sent shutdown already

  shutdownWr(node->socket_fd);
  socketClose(node->socket_fd);
}

int recvMessageContent(message* msg, int socket_fd){
  char buf[LIMIT_DATA_LEN+HEADER_LEN];
  memset(buf, 0, LIMIT_DATA_LEN+HEADER_LEN);
  // 1: read header of ... bytes
  if (!readN(socket_fd, buf, HEADER_LEN)){
    printf("readN failed!\n");
    return 0;
  }
  msg->internal_id=buf[0];
  msg->status_type=buf[1];
  msg->info_type=buf[2];
  msg->response_to=buf[3];
  msg->is_ok=buf[4];
  // current_status ignored!
  /* msg->data_len=buf[4]; */
  /* msg->data_len=buf[5]; */
  /* msg->data_len=buf[6]; */
  char data_len_format[4];
  sprintf(data_len_format, "%%0%dd",
	  CHARS_FOR_DATA_LEN_FIELD);
  sscanf(buf+5, data_len_format, &(msg->data_len));  
  printf("Will read with data len %d\n", msg->data_len);
  if (msg->data_len != 0){
    readN(socket_fd, buf+HEADER_LEN, msg->data_len);
    addData(msg, buf+HEADER_LEN, msg->data_len);
  }
  printMessage(msg);
  if ((msg->data_len == 0) && (msg->data != NULL)){
    printf("Strange msg recv:\n");
    printMessage(msg);
  }
  return 1;
}


int readN(int socket_fd, char* read_buf, int message_len){
  //char tmp_buf[message_len];
  char tmp_buf[LIMIT_DATA_LEN];
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

void finalizeCurrentNode(node_data* node){
  printf("started to finalize current node\n");

  shutdownWr(node->socket_fd);

  waitForThread(&(node->recv_thread));
  printf("node threads finished\n");

  socketClose(node->socket_fd);
  finalizeMessagesSet(&(node->set));

  printf("finalized current node\n");
}

void finalizeServer(server_data* server_params, nodes_info* nodes_params,
		    primes_pool* pool){
  unsigned int server_socket_fd=server_params->listen_socket_fd;
  printf("Shutting down server socket %d\n", server_socket_fd);
  // need to do this to unblock server
  closePendingConnections(nodes_params, pool);
  shutdownRdWr(server_socket_fd);

  printf("Joining accept thread\n");
  waitForThread(&(server_params->accept_thread));
  destroyCondition(&(nodes_params->nodes_refreshed));

  printf("Closing accept socket\n");
  socketClose(server_socket_fd);

  destroyPool(pool);

  destroyMutex(&(nodes_params->nodes_mutex));
}

void closePendingConnections(nodes_info* nodes_params, primes_pool* pool){
  u_condition* signal=&(nodes_params->nodes_refreshed); 
  u_mutex* mutex=&(nodes_params->nodes_mutex);

  lockMutex(mutex);
  int pending=nodes_params->pending_socket;
  if (pending < 0) {
    printf("No pending connections found\n");
    unlockMutex(mutex);
    return;
  }
  printf("Temporary creating and closing pending...\n");
  node_data temp_node;
  int temp_id=10;
  initNewNode(&temp_node, NULL, temp_id, pending, NULL);
  
  message msg;
  fillGeneral(&msg, -1);
  createInitShutdownRequest(&msg, -1);
  message* put_msg=putMessageInSet(msg, &(temp_node.set), TO_SEND, 1);
  waitForThread(&(temp_node.recv_thread));
  finalizeMessagesSet(&(temp_node.set));

  nodes_params->pending_socket=-1;

  signalOne(signal);
  unlockMutex(mutex);
}
