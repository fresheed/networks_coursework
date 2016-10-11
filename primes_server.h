typedef struct node_data {
  unsigned int id;
  pthread_t send_thread, recv_thread;
  unsigned int node_socket_fd;
} node_data;

typedef struct nodes_info {
  unsigned int max_nodes;
  node_data* nodes;
  pthread_mutex_t nodes_mutex;
} nodes_info;

typedef struct server_data {
  pthread_t accept_thread;
  unsigned int listen_socket_fd;
} server_data;

void processAdminInput(server_data* data);
void runAcceptNodes(void* raw_ptr);
int initializeServerData(server_data* data);
int initializeNodes(nodes_info* nodes_struct, int max_nodes, node_data* nodes);
void finalizeServer(server_data* data);


