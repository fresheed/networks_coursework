#include <stdio.h>
#include "general/messages.h"
#include "general/integrity.h"
#include "general/init_sockets.h"
#include "general/common_threads.h"
#include "general/init_sockets.h"
#include "general/messages.h"

int debug_ack_ignore=1;
#define DEBUG_ACK

void initUdpIntegrity(udp_integrity* integrity){
  integrity->cur_recv_id=0;
  integrity->cur_send_id=0;
  integrity->retry_left=-1; // -1 means no retries required yet
  integrity->was_acknowledged=1; // OK initially
  createMutex(&integrity->ack_mutex);
  createCondition(&integrity->updated_ack_status);
}

void finalizeUdpIntegrity(udp_integrity* integrity){
  destroyMutex(&integrity->ack_mutex);
  destroyCondition(&integrity->updated_ack_status);
}

int isAck(message* msg){
  int ack=msg->status_type==ACK;
  if (ack){
    printf("Sending ack, id=%d\n", msg->response_to);
  }
}

// return specifies is msg should be sent
void maintainOutgoingBeforeSend(message* msg, messages_set* set){
  if (msg==NULL || isAck(msg)){ // null check if set inactive already
    return;
  }
  udp_integrity* integrity=&(set->integrity);
  lockMutex(&(integrity->ack_mutex));
  msg->internal_id=integrity->cur_send_id;
  integrity->was_acknowledged=0; // state should become dirty after send

  unlockMutex(&(integrity->ack_mutex));
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
    tryPushAck(set, conn);
    sleepMs(1000);
    printf("Will resend...\n");
  }
  return 0;
}

int tryPushAck(messages_set* set, socket_conn conn){
  u_mutex* mutex=&(set->messages_mutex);
  int result=0;
  lockMutex(mutex);

  int pos=0;
  while ((pos++)<MESSAGES_SET_SIZE){
    message* cur=&(set->messages[pos]);
    if ((cur->status_type==ACK) && (cur->current_status==TO_SEND)){
      // force send ack
      result=sendMessageContent(cur, conn);
      if (result){
	//updateMessageStatus(cur, set, EMPTY_SLOT);
	updatePeerStatus(cur, &(set->integrity));
	cur->current_status=EMPTY_SLOT;
	finalizeMessage(cur);
	signalAll(&(set->status_changed));
      }
      break;
    }
  }

  unlockMutex(mutex);
  return result;
}


int maintainOutgoingAfterSend(message* msg, messages_set* set){
  udp_integrity* integrity=&(set->integrity);
  if (isAck(msg)){
    return updatePeerStatus(msg, integrity);
  } else {
    return waitConfirmed(msg, integrity);
  }
}

int updatePeerStatus(message* msg, udp_integrity* integrity){
  lockMutex(&(integrity->ack_mutex));
  if (integrity->cur_recv_id == msg->response_to){
    integrity->cur_recv_id++;
  }
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
    return validatePeerQuery(msg, set);
  }
}


void checkPeerAnswer(message* msg, udp_integrity* integrity){
  lockMutex(&(integrity->ack_mutex));
  int got_ack=(msg->response_to==integrity->cur_send_id);
  if (got_ack){
    //printf("Received with expected id %d\n", msg->response_to);
#if defined(IS_NODE) && defined(DEBUG_ACK)
    printf("dai: %d\n", debug_ack_ignore);
    if ((debug_ack_ignore++) < 3){
      got_ack=0;
    }
#endif
  } else {
    printf("Wrong ack received: id=%d, send failed\n", msg->response_to);
  }
  integrity->was_acknowledged=got_ack;
  signalAll(&(integrity->updated_ack_status));
  unlockMutex(&(integrity->ack_mutex));
}

int validatePeerQuery(message* msg, messages_set* set){
  udp_integrity* integrity=&(set->integrity);
  char msg_id=msg->internal_id;
  lockMutex(&(integrity->ack_mutex));
  char cur_id=integrity->cur_recv_id;
  int ret_value=1;
  if (msg_id==cur_id){
    if (integrity->was_acknowledged){ // answer only in correct state
      {
	message ack_msg;
	createAckForMessage(&ack_msg, msg_id);
	putMessageInSet(ack_msg, set, TO_SEND, 1);
	//putMessageInSet(ack_msg, set, PENDING_ACK, 1);
      }
    } else {
      printf("Ignoring message because now we have not acknowledged now\n");
      ret_value=0;
    }
  } else if (msg_id==(cur_id-1)){
    // it means peer has not received previous ack
    message ack_msg;
    fillGeneral(&ack_msg, -1);
    createAckForMessage(&ack_msg, cur_id-1);
    putMessageInSet(ack_msg, set, TO_SEND, 1);
    //putMessageInSet(ack_msg, set, PENDING_ACK, 1);
    printf("Send ack again for id %d\n", cur_id-1);
    ret_value=0;
  } else {
    printf("Unexpected msg with id=%d received, cur_recv=%d\n", msg_id, cur_id);
    ret_value=0;
  }

  unlockMutex(&(integrity->ack_mutex));
  return ret_value;
}


/* // ack should be sent independently because otherwise it can be blocked */
/* void forceSendAck(int id_to_confirm, messages_set* set){ */
/*   message ack_msg; */
/*   fillGeneral(&ack_msg, -1); */
/*   createAckForMessage(&ack_msg, id_to_confirm); */
/*   performSend() */
/* } */




