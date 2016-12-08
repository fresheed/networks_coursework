#include <stdio.h>
#include "general/messages.h"
#include "general/integrity.h"

void initUdpIntegrity(udp_integrity* integrity){
  integrity->cur_recv_id=0;
  integrity->cur_send_id=0;
}
  
int isAck(message* msg){
  return msg->status_type==ACK;
}

void maintainOutgoingBeforeSend(message* msg, messages_set* set){
  if (isAck(msg)){
    return;
  }
  udp_integrity* integrity=&(set->integrity);
  msg->internal_id=integrity->cur_send_id;
  lockMutex(&(integrity->ack_mutex));
  integrity->was_acknowledged=0;
  unlockMutex(&(integrity->ack_mutex));
}

void maintainOutgoingAfterSend(message* msg, messages_set* set){
  udp_integrity* integrity=&(set->integrity);
  if (isAck(msg)){
    updatePeerStatus(integrity);
  } else {
    waitConfirmed(msg, integrity);
  }
}

void maintainIncoming(message* msg, messages_set* set){
  udp_integrity* integrity=&(set->integrity);
  if (isAck(msg)){
    checkPeerAnswer(msg, integrity);
  } else {
    validatePeerQuery(msg, set);
  }
}

void updatePeerStatus(udp_integrity* integrity){
  integrity->cur_recv_id++;
}

void waitConfirmed(message* msg, udp_integrity* integrity){
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
}

void checkPeerAnswer(message* msg, udp_integrity* integrity){
  int got_ack=(msg->response_to==integrity->cur_send_id);
  if (got_ack){
    //printf("Received with expected id %d\n", msg->response_to);
  } else {
    printf("Wrong ack received: id=%d, send failed\n", msg->response_to);
  }
  lockMutex(&(integrity->ack_mutex));
  integrity->was_acknowledged=got_ack;
  signalAll(&(integrity->updated_ack_status));
  unlockMutex(&(integrity->ack_mutex));
}

void validatePeerQuery(message* msg, messages_set* set){
  udp_integrity* integrity=&(set->integrity);
  if (msg->internal_id==integrity->cur_recv_id){
    message ack_msg;
    fillGeneral(&ack_msg, -1);
    createAckForMessage(&ack_msg, msg->internal_id);
    message* put_msg=putMessageInSet(ack_msg, set, TO_SEND, 1);
  } else {
    printf("Unexpected msg with id=%d received\n", msg->internal_id);    
  }
}








