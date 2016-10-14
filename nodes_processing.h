#include "primes_server.h"

void addNewNode(nodes_info* nodes_params, int new_socket_fd);
void closePendingConnections(nodes_info* nodes_params);
void finalizeNodes(nodes_info* nodes_params);
int getNewNodeIndex(node_data* nodes, int count);
void initNewNode(node_data* node, unsigned int id, unsigned int fd);
