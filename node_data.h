struct node_data {
  unsigned int id;
  unsigned int socket_fd;
  pthread_t send_thread;
  pthread_t recv_thread;
};
