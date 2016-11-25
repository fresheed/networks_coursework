#include "general/messages.h"
#include "server/primes_server.h"
#include "general/logic.h"

int sendMessageContent(message* msg, socket_conn conn);
void endCommunication(node_data* node);
int recvMessageContent(message* msg, socket_conn conn);
int readN(int socket_fd, char* read_buf, int message_len);
void finalizeServer(server_data* server_params, nodes_info* nodes_params,
		    primes_pool* pool);
void closePendingConnections(nodes_info* nodes_params, primes_pool* pool);
void finalizeCurrentNode(node_data* node);
