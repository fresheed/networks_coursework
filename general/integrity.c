#include <stdio.h>
#include "general/messages.h"
#include "general/integrity.h"
#include "general/init_sockets.h"
#include "general/common_threads.h"
#include "general/init_sockets.h"
#include "general/messages.h"

void initUdpIntegrity(udp_integrity* integrity){
  integrity->cur_recv_id=0;
  integrity->cur_send_id=0;
  integrity->retry_left=-1; // -1 means no retries required yet
  integrity->was_acknowledged=1; // OK initially
}

int isAck(message* msg){
    printf("is ack?\n");
  return msg->status_type==ACK;
}

// return specifies is msg should be sent
void maintainOutgoingBeforeSend(message* msg, messages_set* set){
  if (msg==NULL || isAck(msg)){ // null check if set inactive already
    printf("ack - ret\n");
    return;
  }
      printf("c1");
  udp_integrity* integrity=&(set->integrity);
  lockMutex(&(integrity->ack_mutex));
  msg->internal_id=integrity->cur_send_id;
      printf("c1");
  integrity->was_acknowledged=0; // state should become dirty after send

  unlockMutex(&(integrity->ack_mutex));
      printf("c1");
}

int performSend(message* msg, messages_set* set, socket_conn conn){
  int retry_count=RETRY_ATTEMPTS;
  while (retry_count-- > 0){
    int tech_sent=sendMessageContent(msg, conn);
    if (!tech_sent){ // technical problems, return immediately
      return 0;
    }
    if (msg->info_type==INIT_SHUTDOWN) {
      return 0; // no ack wait for shutdown
    }
    if (maintainOutgoingAfterSend(msg, set)){ // received ack or msg is ack
      return 1;
    }
    sleepMs(1000);
  }
  return 0;
}


int maintainOutgoingAfterSend(message* msg, messages_set* set){
  udp_integrity* integrity=&(set->integrity);
  if (isAck(msg)){
    return updatePeerStatus(integrity);
  } else {
    return waitConfirmed(msg, integrity);
  }
}

int updatePeerStatus(udp_integrity* integrity){
  lockMutex(&(integrity->ack_mutex));
  integrity->cur_recv_id++;
  unlockMutex(&(integrity->ack_mutex));
  return 1;
}

int waitConfirmed(message* msg, udp_integrity* integrity){
  lockMutex(&(integrity->ack_mutex));
  // checkPeerAnswer must be executed now
  if (!integrity->was_acknowledged){
    blockWithTimeout(&(integrity->updated_ack_status),
		     &(integrity->ack_mutex),
		     ACK_WAIT_PERIOD*1000);
  }
  if (integrity->was_acknowledged){
    printf("Send for id=%d acknowledged\n", integrity->cur_send_id);
    integrity->cur_send_id++;
  } else {
    printf("Failed to send - ack for id=%d not received!\n",  integrity->cur_send_id);
  }
  unlockMutex(&(integrity->ack_mutex));
  return integrity->was_acknowledged;
}


int maintainIncoming(message* msg, messages_set* set){
  udp_integrity* integrity=&(set->integrity);
  if (isAck(msg)){
    checkPeerAnswer(msg, integrity);
    return 0;
  } else {
    validatePeerQuery(msg, set);
    return 1;
  }
}


void checkPeerAnswer(message* msg, udp_integrity* integrity){
  lockMutex(&(integrity->ack_mutex));
  int got_ack=(msg->response_to==integrity->cur_send_id);
  if (got_ack){
    //printf("Received with expected id %d\n", msg->response_to);
  } else {
    printf("Wrong ack received: id=%d, send failed\n", msg->response_to);
  }
  integrity->was_acknowledged=got_ack;
  signalAll(&(integrity->updated_ack_status));
  unlockMutex(&(integrity->ack_mutex));
}

void validatePeerQuery(message* msg, messages_set* set){
  udp_integrity* integrity=&(set->integrity);
  char msg_id=msg->internal_id;
  lockMutex(&(integrity->ack_mutex));
  char cur_id=integrity->cur_recv_id;
  if (msg_id==cur_id){
    if (integrity->was_acknowledged){ // answer only in correct state
      message ack_msg;
      createAckForMessage(&ack_msg, msg_id);
      putMessageInSet(ack_msg, set, TO_SEND, 1);
    } else {
      printf("Ignoring message because now we have not acknowledged now\n");
    }
  } else if (msg_id==(cur_id-1)){
      // it means peer has not received previous ack
      message ack_msg;
      fillGeneral(&ack_msg, -1);
      createAckForMessage(&ack_msg, cur_id-1);
      putMessageInSet(ack_msg, set, TO_SEND, 1);
  } else {
    printf("Unexpected msg with id=%d received\n", msg_id);
  }

  unlockMutex(&(integrity->ack_mutex));
}








