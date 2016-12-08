#include <stdio.h>
#include "general/messages.h"
#include "general/integrity.h"

/* void initUdpIntegrity(udp_integrity* integrity){ *\/ */
/*   integrity->id_to_recv=0; */
/*   integrity->id_to_send=0; */
/* } */

/* int isAck(message* msg){ */
/*   return msg->status_type==ACK; */
/* } */

void maintainOutgoingBeforeSend(message* msg, messages_set* set){
  printf("maintaining before send...\n");
}

int maintainOutgoingAfterSend(message* msg, messages_set* set){
  printf("maintaining after send...\n");
}

void maintainIncoming(message* msg, messages_set* set){
  printf("maintaining received...\n");
}

