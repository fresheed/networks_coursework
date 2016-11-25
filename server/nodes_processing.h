#include "server/primes_server.h"
#include "general/logic.h"

void addNewNode(nodes_info* nodes_params, int new_socket_fd, primes_pool* pool);
//void closePendingConnections(nodes_info* nodes_params, primes_pool* pool);
void finalizeNodes(nodes_info* nodes_params);
int getNewNodeIndex(node_data* nodes, int count);
void initNewNode(node_data* node, nodes_info* nodes_params, unsigned int id, unsigned int fd, primes_pool* pool);
void kickSingleNode(nodes_info* nodes_params, int id);
void processKick(nodes_info* nodes_params, int id);
void cleanupZombieNodes(nodes_info* nodes_params);
void printNodes(nodes_info* nodes_params);
