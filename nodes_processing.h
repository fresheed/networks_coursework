#include "primes_server.h"

void addNewNode(nodes_info* nodes_params, int new_socket_fd);
void closePendingConnections(nodes_info* nodes_params);
void finalizeNodes(nodes_info* nodes_params);
int getNewNodeIndex(node_data* nodes, int count);
void initNewNode(node_data* node, nodes_info* nodes_params, unsigned int id, unsigned int fd);
void closeDisconnectedNodes(nodes_info* nodes_params);
