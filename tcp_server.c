#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
#include "init_sockets.h"
#include "logs.h"
#include "client_data.h"

char errbuf[300];
void on_error(char *msg){
  printf(msg); 
  printf("\nErrno:  %d\n", errno);
  perror(errbuf);
  printf(errbuf);
  exit(1);
}

void closeFdAbnormally(int fd, char* msg){
  close(fd);
  on_error(msg);
}

int readN(int socket_fd, char* read_buf){
  const int message_len=4;
  char tmp_buf[message_len];
  const int recv_flags=0;
  int total_read=0;

  memset(read_buf, 0, message_len);
  int read_status=0;
  while (total_read < message_len){
    memset(tmp_buf, 0, message_len);
    int to_read_now=message_len-total_read;
    int actual_read_now=recv(socket_fd, tmp_buf, to_read_now, recv_flags);
    printf("Read fragment: %s\n", tmp_buf);
    if (actual_read_now < 0){
      threadLog("Error on reading message data, exiting...");
      read_status=1;
      break;
    }
    total_read+=actual_read_now;
    strcat(read_buf, tmp_buf);
  }
  return read_status==0;
}

int sendResponse(char* msg, int client_socket_fd){
  static char response[20];
  strcpy(response, "Received:");
  strcat(response, msg);
  int num_bytes_processed=send(client_socket_fd, response, strlen(response), 0);
  return (num_bytes_processed>=0);
}

void* processClient(void* arg){
  // at this point client is already connected
  int client_socket_fd=*(int*)arg;
  char buffer[256];
  do {
    if (!readN(client_socket_fd, buffer)){
      printf("Client closed connection\n");
      break;
    }
    threadLog();
    printf("Client %d message: %s\n",client_socket_fd,  buffer);
    if (!sendResponse(buffer, client_socket_fd)){
      printf( "Client closed connection\n");
      break;
    }
  } while ((strcmp(buffer, "exit")));// && num_bytes_processed);
  threadLog();
  printf("Exiting thread\n");
  pthread_exit(NULL);
}

#define MAX_CLIENTS 3
struct client_data clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex;
int clients_count;
void runAcceptClients(int server_socket_fd){
  /* int clients_fds[MAX_CLIENTS]; */
  /* pthread_t clients_threads[MAX_CLIENTS]; */
  clients_count=0;
  while(clients_count<MAX_CLIENTS){
    int tmp_fd=acceptClient(server_socket_fd);
    if (tmp_fd > 0){
      pthread_mutex_lock(&clients_mutex);
      sprintf(&clients[clients_count].name, "Client %d", clients_count);
      clients[clients_count].socket_fd=tmp_fd;
      pthread_create(&(clients[clients_count].thread), NULL, &processClient, &(clients[clients_count].socket_fd));
      threadLog();
      printf("Created client thread\n");
      clients_count++;
      pthread_mutex_unlock(&clients_mutex);
    } else {
      threadLog();
      printf("Accept failed\n");
      break;
    }
  }
  
  // after max_clients this func goes here immediately and finished threads
  // last thread could even not send anything
  threadLog();
  printf("Closing client sockets...\n");
  int i;
  pthread_mutex_lock(&clients_mutex);
  for (i=0; i<clients_count; i++){
    close(clients[i].socket_fd);
  }
  
  threadLog();
  printf("Joining client threads...\n");
  for (i=0; i<clients_count; i++){
    pthread_join(clients[i].thread, NULL);
    printf("Joined client thread %d\n", clients[i].thread);
  } 
  pthread_mutex_unlock(&clients_mutex);
}

void listClients(){
  int i;
  printf("Connected clients:\n");
  pthread_mutex_lock(&clients_mutex);
  for (i=0; i<clients_count; i++){
    struct client_data client=clients[i];
    printf("%s, socket: %d, thread: %d\n", client.name, client.socket_fd, client.thread );
  }
  pthread_mutex_unlock(&clients_mutex);
}

void kickClient(int id){

  pthread_mutex_lock(&clients_mutex);
  if (id >= clients_count){
    printf("No such client\n");
    return;
  }
  close(clients[id].socket_fd);
  pthread_join(clients[id].thread, NULL);
  printf("Kicked client %d\n", id);
  pthread_mutex_unlock(&clients_mutex);
}



int main(int argc, char *argv[]){

  int server_socket_fd=prepareServerSocket();
  
  pthread_mutex_init(&clients_mutex, NULL);

  pthread_t accept_thread;
  pthread_create(&accept_thread, NULL, &runAcceptClients, server_socket_fd);

  char admin_input[100];
  while (1){
    printf("\n=>");
    //scanf("%s", admin_input);
    fgets(admin_input, 100, stdin);
    if (strncmp(admin_input, "q", 1)==0){
      printf("QUIT\n");
      break;
    } else if (strncmp(admin_input, "l", 1)==0){
      listClients();
    } else if (strncmp(admin_input, "k ", 2)==0){
      // format: k (id)
      char id_raw=admin_input[2];
      if (isdigit(id_raw)){
	int id_to_kick=id_raw-'0';
	kickClient(id_to_kick);
      } else {
	printf("Wrong format. Try k (id 0..9)\n");
      }
    }
  }
  
  printf("Closing server socket %d\n", server_socket_fd);
  shutdown(server_socket_fd, SHUT_RDWR);
  int rescode=close(server_socket_fd);
  printf("close ret code: %d\n", rescode);
  printf("Joining accept thread\n");
  pthread_join(accept_thread, NULL);
  printf("Server stopped\n");

  pthread_mutex_destroy(&clients_mutex);

  return 0;
}

