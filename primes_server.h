#include <pthread.h>
#include "messages.h"

#ifndef server_structs
#define server_structs
typedef struct node_data {
  unsigned int id;
  pthread_t send_thread, recv_thread;
  messages_set set;
  unsigned int socket_fd;
  int test;
} node_data;

typedef struct nodes_info {
  unsigned int max_nodes;
  node_data* nodes;
  unsigned int unique_id_counter;
  pthread_mutex_t nodes_mutex;
  pthread_cond_t nodes_refreshed;
  unsigned int pending_socket;
} nodes_info;

typedef struct server_data {
  pthread_t accept_thread;
  unsigned int listen_socket_fd;
} server_data;
#endif

void processAdminInput();
void* runAcceptNodes();
int initializeServer();
void finalizeServer();


