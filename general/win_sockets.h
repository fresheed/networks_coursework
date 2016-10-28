#include <sys/types.h>


void setServerAddressParams(struct sockaddr_in* server_address, int port);
int prepareServerSocket();
int acceptClient(int server_socket_fd);
void shutdownWr(int socket_fd);
void shutdownRdWr(int socket_fd);
