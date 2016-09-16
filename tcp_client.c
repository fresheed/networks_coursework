#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h> 
#include <errno.h>

void on_error(char *msg){
  printf(msg);
  exit(1);
}

void exchangeData(int client_socket_fd, char* msg){
  char buffer[256];
  int num_bytes_read = write(client_socket_fd, msg, strlen(msg));
  if (num_bytes_read < 0) 
    on_error("ERROR writing to socket");
  bzero(buffer,256);
  num_bytes_read = read(client_socket_fd,buffer,255);
  if (num_bytes_read < 0) 
    on_error("ERROR reading from socket");
  printf("%s\n",buffer);
  
}

int main(int argc, char* argv[]){
  const int server_port=3451;
  int client_socket_fd;
  int num_bytes_read;
  struct sockaddr_in server_address;
  struct hostent* server;
  
  if (argc < 2) {
    fprintf(stderr,"usage: %s hostname \n", argv[0]);
    exit(0);
  }
  client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket_fd < 0) 
    on_error("ERROR opening socket");
  server=gethostbyname(argv[1]);
  if (server==NULL){
    on_error("Host  not found");
  }

  bzero((char *) &server_address, sizeof(server_address));
  server_address.sin_family=AF_INET;
  bcopy((char*)server->h_addr, (char*)&server_address.sin_addr.s_addr,
	server->h_length);
  server_address.sin_port=htons(server_port);

  if (connect(client_socket_fd,(struct sockaddr *)&server_address, sizeof(server_address)) < 0) 
    on_error("ERROR connecting\n");

  int i;
  for (i=0; i<10; i++){
    exchangeData(client_socket_fd, "aaaa");
    sleep(2);
  }

  close(client_socket_fd);
  return 0;  
}
