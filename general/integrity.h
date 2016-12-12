#include "general/messages.h"
#include "general/init_sockets.h"

#ifndef integrity_structs
#define integrity_structs

#define ACK_WAIT_PERIOD 3
#define RETRY_ATTEMPTS 3

#endif

void initUdpIntegrity(udp_integrity* integrity);
void finalizeUdpIntegrity(udp_integrity* integrity);
int isAck(message* msg);
void maintainOutgoingBeforeSend(message* msg, messages_set* set);

int performSend(message* msg, messages_set* set, socket_conn conn);
int maintainOutgoingAfterSend(message* msg, messages_set* set);
int maintainIncoming(message* msg, messages_set* set);

int updatePeerStatus(udp_integrity* integrity);
int waitConfirmed(message* msg, udp_integrity* integrity);
void checkPeerAnswer(message* msg, udp_integrity* integrity);
void validatePeerQuery(message* msg, messages_set* set);
