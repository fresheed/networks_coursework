//#include <pthread.h>
#include "general/u_threads.h"
#include "general/messages.h"
#include "general/logic.h"
#include "general/u_threads.h"

#ifndef server_structs
#define server_structs

typedef struct node_data {
  unsigned int id;
//  pthread_t send_thread, recv_thread, proc_thread;
    u_thread send_thread, recv_thread, proc_thread;
  messages_set set;
  unsigned int socket_fd;
  //void* nodes_params;
  primes_pool* common_pool;
} node_data;

typedef struct nodes_info {
  unsigned int max_nodes;
  node_data* nodes;
  unsigned int unique_id_counter;
  //pthread_mutex_t nodes_mutex;
  //pthread_cond_t nodes_refreshed;
u_mutex nodes_mutex;
  u_condition nodes_refreshed;
  unsigned int pending_socket;
} nodes_info;

typedef struct server_data {
 // pthread_t accept_thread;
 u_thread accept_thread;
  unsigned int listen_socket_fd;
  unsigned int last_executor;
} server_data;
#endif

void processAdminInput();
void* runAcceptNodes();
int initializeServer();
void finalizeServer();


