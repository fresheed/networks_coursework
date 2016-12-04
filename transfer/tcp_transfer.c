#include "transfer/tcp_transfer.h"
#include "server/nodes_processing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int sendBytesToPeer(socket_conn conn, char* data_buffer, int send_len){
  // TCP: just send bytes to corresponding socket
  const int send_flags=0;
  return send(conn.socket_fd, data_buffer, send_len, send_flags);
}

int recvBytesFromPeer(socket_conn conn, char* read_buffer, int to_read){
  // TCP: just read bytes from socket
  const int recv_flags=0;
  return recv(conn.socket_fd, read_buffer, to_read, recv_flags);
}

// should be executed from recv thread only
void endCommunication(node_data* node){
  waitForThread(&(node->proc_thread));
  waitForThread(&(node->send_thread));

  // at this point peer should sent shutdown already

  shutdownWr(node->conn.socket_fd);
  socketClose(node->conn.socket_fd);
}


void finalizeCurrentNode(node_data* node){
  printf("started to finalize current node\n");

  shutdownWr(node->conn.socket_fd);

  waitForThread(&(node->recv_thread));
  printf("node threads finished\n");

  socketClose(node->conn.socket_fd);
  finalizeMessagesSet(&(node->set));

  printf("finalized current node\n");
}

void finalizeServer(server_data* server_params, nodes_info* nodes_params,
		    primes_pool* pool){
  socket_conn server_conn=server_params->listen_conn;
  printf("Shutting down server socket %d\n", server_conn.socket_fd);
  // need to do this to unblock server
  closePendingConnections(nodes_params, pool);
  shutdownRdWr(server_conn.socket_fd);

  printf("Joining accept thread\n");
  waitForThread(&(server_params->accept_thread));
  destroyCondition(&(nodes_params->nodes_refreshed));

  printf("Closing accept socket\n");
  socketClose(server_conn.socket_fd);

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
  message* put_msg=putMessageInSet(msg, &(temp_node.set), TO_SEND, 1);
  waitForThread(&(temp_node.recv_thread));
  finalizeMessagesSet(&(temp_node.set));

  nodes_params->pending_conn=err_socket;

  signalOne(signal);
  unlockMutex(mutex);
}
