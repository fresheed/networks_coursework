#include "general/u_threads.h"
#include "general/messages.h"
#include "general/logic.h"
#include "general/u_threads.h"
#include "general/init_sockets.h"

#ifndef server_structs
#define server_structs

typedef struct node_data {
  unsigned int id;
  u_thread send_thread, recv_thread, proc_thread;
  messages_set set;
  socket_conn conn;
  primes_pool* common_pool;
} node_data;

typedef struct nodes_info {
  unsigned int max_nodes;
  node_data* nodes;
  unsigned int unique_id_counter;
  u_mutex nodes_mutex;
  u_condition nodes_refreshed;
  socket_conn pending_conn;
} nodes_info;

typedef struct server_data {
  u_thread accept_thread;
  socket_conn listen_conn;
  unsigned int last_executor;
} server_data;
#endif

void processAdminInput();
void* runAcceptNodes();
int initializeServer();
void finalizeServer();


