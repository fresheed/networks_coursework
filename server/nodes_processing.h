#include "server/primes_server.h"
#include "general/logic.h"
#include "general/init_sockets.h"

void addNewNode(nodes_info* nodes_params, socket_conn new_conn, primes_pool* pool);
//void closePendingConnections(nodes_info* nodes_params, primes_pool* pool);
void finalizeNodes(nodes_info* nodes_params);
int getNewNodeIndex(node_data* nodes, int cnt);
void initNewNode(node_data* node, nodes_info* nodes_params, unsigned int id, socket_conn conn, primes_pool* pool);
void kickSingleNode(nodes_info* nodes_params, int id);
void processKick(nodes_info* nodes_params, int id);
void cleanupZombieNodes(nodes_info* nodes_params);
void printNodes(nodes_info* nodes_params);
int assignTaskToNextNode(int last_executor, long lower_bound, long upper_bound, nodes_info* nodes_params);
int getIndexByAddress(node_data* nodes, int count, struct sockaddr_in address);
int getIndexById(node_data* nodes, int cnt, int id);
