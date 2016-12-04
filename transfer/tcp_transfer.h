#include "general/messages.h"
#include "server/primes_server.h"
#include "general/logic.h"

int sendBytesToPeer(socket_conn conn, char* data_buffer, int send_len);
void endCommunication(node_data* node);
void finalizeServer(server_data* server_params, nodes_info* nodes_params,
		    primes_pool* pool);
void closePendingConnections(nodes_info* nodes_params, primes_pool* pool);
void finalizeCurrentNode(node_data* node);
int sendBytesToPeer(socket_conn conn, char* data_buffer, int send_len);
int recvBytesFromPeer(socket_conn conn, char* read_buffer, int to_read);
