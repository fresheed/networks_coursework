
#ifndef integrity_structs
#define integrity_structs

#include "general/messages.h"

typedef struct {
  char id_to_recv;
  char id_to_send;
  
} udp_integrity;

#endif

/* void initUdpIntegrity(udp_integrity* integrity); */
/* int isAck(message* msg); */
void maintainOutgoingBeforeSend(message* msg, messages_set* set);
int maintainOutgoingAfterSend(message* msg, messages_set* set);
void maintainIncoming(message* msg, messages_set* set);
