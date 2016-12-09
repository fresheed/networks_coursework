#include "general/messages.h"
#include "server/primes_server.h"
#include "general/logic.h"

int sendBytesToPeer(socket_conn conn, char* data_buffer, int send_len);
int recvBytesFromPeer(socket_conn conn, char* read_buffer, int to_read);
int putDatagramToPipe(socket_conn conn, char* read_buffer, int to_read);

void endCommunication(node_data* node);
void finalizeServer(server_data* server_params, nodes_info* nodes_params,
		    primes_pool* pool);
void closePendingConnections(nodes_info* nodes_params, primes_pool* pool);
void finalizeCurrentNode(node_data* node);
void stopMessageThreads(node_data* node);

void printAddress(struct sockaddr_in address);
void endCommunication(node_data* node);
void stopNodeThreads(node_data* node);
void stopServerThreadsForNode(node_data* node);
