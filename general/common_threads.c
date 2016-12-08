#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server/primes_server.h"
#include "general/init_sockets.h"
#include "server/server_threads.h"
#include "general/messages.h"
#include "general/common_threads.h"
#include "transfer/net_transfer.h"
#include "general/integrity.h"

void* common_send_thread(void* raw_node_ptr){
  node_data* node=(node_data*)raw_node_ptr;
  socket_conn conn=node->conn;
  messages_set* set=&(node->set);
  int id=node->id;
  while(1){
    printf("locked...\n");
    message* msg=lockNextMessage(set, TO_SEND); // now in OWNED state
    if (!set->is_active){
      printf("Message set is unactive, stopping to send\n");
      break;
    }
    maintainOutgoingBeforeSend(msg, set);
    printf("sending...\n");
    int send_result=sendMessageContent(msg, conn);
    if (!send_result){
      printf("Send to node %d failed", id);
      break;
    } else {
      maintainOutgoingAfterSend(msg, set);
      char next_status=(msg->status_type == REQUEST) ? WAITS_RESPONSE : EMPTY_SLOT;
      updateMessageStatus(msg, set, next_status);
    }
  }
  printf("Stopped to send to node %d\n", id);
  return NULL;
}

int sendMessageContent(message* msg, socket_conn conn){
  char buf[LIMIT_DATA_LEN+HEADER_LEN];
  int needed_len=HEADER_LEN, actual_sent;
  memset(buf, 0, LIMIT_DATA_LEN+HEADER_LEN);

  buf[0]=msg->internal_id;
  buf[1]=msg->status_type;
  buf[2]=msg->info_type;
  buf[3]=msg->response_to;
  buf[4]=msg->is_ok;
  sprintf(buf+5, "%06d", msg->data_len);

  if (msg->data != NULL){
    memcpy(buf+HEADER_LEN, msg->data, msg->data_len);
    needed_len+=msg->data_len;
  }
  if ((msg->data_len == 0) && (msg->data != NULL)){
    printf("Strange msg send:\n");
    printMessage(msg);
  }

  actual_sent=sendBytesToPeer(conn, buf, needed_len);
  return (actual_sent == needed_len);
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
      maintainIncoming(&msg, set);
      message* put_msg=putMessageInSet(msg, set, TO_PROCESS, 0);
      if (!set->is_active){
	printf("Message set is unactive, stopping to receive\n");
	break;
      }
    }
  }
  printf("Stopped to receive from node %d\n", id);
  endCommunication(node);
  return NULL;
}

int recvMessageContent(message* msg, socket_conn conn){
  char buf[LIMIT_DATA_LEN+HEADER_LEN];
  memset(buf, 0, LIMIT_DATA_LEN+HEADER_LEN);
  // 1: read header of ... bytes
  if (!readN(conn, buf, HEADER_LEN)){
    printf("readN failed!\n");
    return 0;
  }
  msg->internal_id=buf[0];
  msg->status_type=buf[1];
  msg->info_type=buf[2];
  msg->response_to=buf[3];
  msg->is_ok=buf[4];

  char data_len_format[4];
  sprintf(data_len_format, "%%0%dd",
	  CHARS_FOR_DATA_LEN_FIELD);
  sscanf(buf+5, data_len_format, &(msg->data_len));  
  if (msg->data_len != 0){
    readN(conn, buf+HEADER_LEN, msg->data_len);
    addData(msg, buf+HEADER_LEN, msg->data_len);
  }
  if ((msg->data_len == 0) && (msg->data != NULL)){
    printf("Strange msg recv:\n");
    printMessage(msg);
  }
  return 1;
}


int readN(socket_conn conn, char* read_buf, int message_len){
  //char tmp_buf[message_len];
  char tmp_buf[LIMIT_DATA_LEN];
  const int recv_flags=0;
  int total_read=0;
  memset(read_buf, 0, message_len);
  int read_status=0;
  while (total_read < message_len){
    memset(tmp_buf, 0, message_len);
    int to_read_now=message_len-total_read;
    int actual_read_now=recvBytesFromPeer(conn, tmp_buf, to_read_now);
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
