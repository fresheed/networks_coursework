#include "WinSock2.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

void libraryInit(){
	WSADATA WsaData;
	int err = WSAStartup (0x0101, &WsaData);
	if (err == SOCKET_ERROR)	{
           printf ("WSAStartup() failed: %ld\n", GetLastError ());
           exit(1);
	}
}

void on_error(char* msg){
    printf(msg);
    exit(1);
}

void exchangeData(int client_socket_fd, char* msg){
  char buffer[256];
  int num_bytes_read = send(client_socket_fd, msg, strlen(msg), 0);
  if (num_bytes_read < 0)
    on_error("ERROR writing to socket");
  memset(buffer, 0, 256);
  num_bytes_read = recv(client_socket_fd,buffer,255, 0);
  if (num_bytes_read < 0)
    on_error("ERROR reading from socket");
  printf("%s\n",buffer);
}

void sendData(int client_socket_fd, char* msg){
  char buffer[256];
  int num_bytes_read = send(client_socket_fd, msg, strlen(msg), 0);
  if (num_bytes_read < 0)
    on_error("ERROR writing to socket");
}



void run_client(){
    printf("run client\n");
    libraryInit();
    int client_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (client_socket < 0) on_error("Socket creation failed\n");

	SOCKADDR_IN server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(3451);
	server_address.sin_addr.S_un.S_addr = inet_addr("192.168.56.1");

    connect(client_socket, (struct sockaddr *)&server_address, sizeof(struct sockaddr));

    sendData(client_socket, "a");
    Sleep(1000);
    sendData(client_socket, "bcd");
    Sleep(1000);
    sendData(client_socket, "abcda");
    Sleep(1000);
    sendData(client_socket, "bcd");
    Sleep(1000);
    sendData(client_socket, "ab");
    Sleep(1000);
    sendData(client_socket, "cd");
    Sleep(1000);

    close(client_socket);

    return;

}

