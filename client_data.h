struct client_data {
  char name[20];
  unsigned int socket_fd;
  pthread_t thread;
};
