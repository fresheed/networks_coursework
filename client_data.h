struct client_data {
  unsigned int socket_fd;
  pthread_t thread;
  pthread_mutex_t mutex;
};
