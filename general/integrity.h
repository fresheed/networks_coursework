#include "general/messages.h"


#ifndef integrity_structs
#define integrity_structs

#define ACK_WAIT_PERIOD 5

#endif

void initUdpIntegrity(udp_integrity* integrity);
int isAck(message* msg);
void maintainOutgoingBeforeSend(message* msg, messages_set* set);
void maintainOutgoingAfterSend(message* msg, messages_set* set);
void maintainIncoming(message* msg, messages_set* set);

void updatePeerStatus(udp_integrity* integrity);
void waitConfirmed(message* msg, udp_integrity* integrity);
void checkPeerAnswer(message* msg, udp_integrity* integrity);
void validatePeerQuery(message* msg, messages_set* set);
