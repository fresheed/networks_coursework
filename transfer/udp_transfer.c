#include "transfer/udp_transfer.h"
#include "server/nodes_processing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <poll.h>


int sendBytesToPeer(socket_conn conn, char* data_buffer, int send_len){
  // UDP: send is done directly with sendto
  const int send_flags=0;
//  char test_buf[]="Hello";
//    return sendto(conn.socket_fd, test_buf, 5,
//		0, (struct sockaddr*) &conn.peer_address,
//		sizeof(struct sockaddr));
  return sendto(conn.socket_fd, data_buffer, send_len,
		send_flags, (struct sockaddr*) &conn.peer_address,
		sizeof(struct sockaddr));
}

int recvBytesFromPeer(socket_conn conn, char* read_buffer, int to_read){
  // UDP: all data is first read into pipe
  // then it goes here
#ifdef IS_SERVER
  // server: data goes into pipe from common recv thread
  return readFromPipe(conn.pipe_out_fd, read_buffer, to_read);
#endif
#ifdef IS_NODE
  // node: recv thread also reads into pipe
  // also we should assert who sent data
  return putDatagramToPipe(conn, read_buffer, to_read);
#endif
}

int putDatagramToPipe(socket_conn conn, char* read_buffer, int to_read){
  const int recv_flags=0;
  struct sockaddr_in sender;
  int addr_len=sizeof(sender);
  int to_read_left=to_read;
  int read_from_pipe=0;
  // if some data left in pipe, read it first
  int pipe_has_data=pipeHasData(conn.pipe_out_fd);
  if (pipe_has_data==1) {
    read_from_pipe=readFromPipe(conn.pipe_out_fd, read_buffer, to_read);
    if ((read_from_pipe == to_read) || (read_from_pipe == -1)){
      return read_from_pipe;
    } else {
      to_read_left-=read_from_pipe;
    }
  }

  char recv_buffer[LIMIT_DATA_LEN];
  int recv_len=recvfrom(conn.socket_fd, recv_buffer,
			LIMIT_DATA_LEN, recv_flags,
			(struct sockaddr*)&sender,
			&addr_len);
  writeToPipe(conn.pipe_in_fd, recv_buffer, recv_len);
  if (!(addressesAreEqual(sender, conn.peer_address))){
    printf("Sender unknown:\n");
    printAddress(sender);
    return -1;
  } else {
    int read_from_pipe_2=readFromPipe(conn.pipe_out_fd,
				      read_buffer+read_from_pipe,
				      to_read_left);
    if ((read_from_pipe_2==to_read_left) || (read_from_pipe_2==-1)){
      return to_read;
    } else {
      return -1;
    }
  }
}

// should be executed from recv thread only
void endCommunication(node_data* node){
  //stopMessageThreads(node);
  waitForThread(&(node->proc_thread));
  waitForThread(&(node->send_thread));
}

void stopMessageThreads(node_data* node){
#ifdef IS_SERVER
  stopServerThreadsForNode(node);
#endif
#ifdef IS_NODE
  stopNodeThreads(node);
#endif
}

void stopNodeThreads(node_data* node){
  // recv reads from socket and pipe, so need to close both
  shutdownRdWr(node->conn.socket_fd);
  closePipeDescriptor(node->conn.pipe_in_fd); // may be already closed
  closePipeDescriptor(node->conn.pipe_out_fd);
}

void stopServerThreadsForNode(node_data* node){
  // recv reads from pipe only, so close only pipe
  closePipeDescriptor(node->conn.pipe_in_fd); // may be already closed
  closePipeDescriptor(node->conn.pipe_out_fd);
}

void notifyServerAboutShutdown(node_data* node){
  message msg;
  fillGeneral(&msg, -1);
  createInitShutdownRequest(&msg, -1);
  putMessageInSet(msg, &(node->set), TO_SEND, 1);
}

void finalizeCurrentNode(node_data* node){
  printf("started to finalize current node\n");
  notifyServerAboutShutdown(node);
  /* shutdownWr(node->conn); */
  //stopNodeThreads(node);
  //shutdownRdWr(node->conn.socket_fd);

  waitForThread(&(node->recv_thread));
  printf("node threads finished\n");

  finalizeMessagesSet(&(node->set));

  socketClose(node->conn.socket_fd);

  printf("finalized current node\n");

}

void finalizeServer(server_data* server_params, nodes_info* nodes_params,
		    primes_pool* pool){
  socket_conn server_conn=server_params->listen_conn;
  printf("shutting down and closing accept socket\n");
  //shutdownRdWr(server_conn.socket_fd);
  shutdownRd(server_conn.socket_fd);

  waitForThread(&(server_params->accept_thread));

  shutdownWr(server_conn.socket_fd);
  socketClose(server_conn.socket_fd);

  destroyCondition(&(nodes_params->nodes_refreshed));
  destroyPool(pool);
  destroyMutex(&(nodes_params->nodes_mutex));
}

void closePendingConnections(nodes_info* nodes_params, primes_pool* pool){
  u_condition* signal=&(nodes_params->nodes_refreshed);
  u_mutex* mutex=&(nodes_params->nodes_mutex);

  lockMutex(mutex);
  socket_conn pending_conn=nodes_params->pending_conn;
  if (pending_conn.socket_fd < 0) {
    printf("No pending connections found\n");
    unlockMutex(mutex);
    return;
  }
  printf("Temporary creating and closing pending...\n");
  node_data temp_node;
  int temp_id=10;
  initNewNode(&temp_node, NULL, temp_id, pending_conn, NULL);

  message msg;
  fillGeneral(&msg, -1);
  createInitShutdownRequest(&msg, -1);
  putMessageInSet(msg, &(temp_node.set), TO_SEND, 1);
  waitForThread(&(temp_node.recv_thread));
  finalizeMessagesSet(&(temp_node.set));

  nodes_params->pending_conn=err_socket;

  signalOne(signal);
  unlockMutex(mutex);
}

void printAddress(struct sockaddr_in address){
  char to_print[30];
  memset(to_print, 0, 30);
#ifdef _WIN32
  printf("Port:%d\n", address.sin_port);
#else
  inet_ntop(AF_INET, &(address.sin_addr), to_print, INET_ADDRSTRLEN);
  printf("Address: %s:%d\n", to_print, address.sin_port);
#endif
}
