//#include <pthread.h>
#include "general/messages.h"
#include "general/init_sockets.h"

#ifndef node_structs
#define node_structs

#endif

void finalizeCurrentNode();

int initializeCurrentNode(socket_conn conn);

void processUserInput();
