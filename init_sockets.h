#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


void setServerAddressParams(struct sockaddr_in* server_address, int port);
int prepareServerSocket();
int acceptClient(int server_socket_fd);
