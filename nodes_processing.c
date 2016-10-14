#include <pthread.h>
#include <stdio.h>
#include "nodes_processing.h"
#include "nodes_threads.h"


void addNewNode(nodes_info* nodes_params, int new_socket_fd){
  node_data* nodes=nodes_params->nodes;
  pthread_mutex_t mutex=nodes_params->nodes_mutex;
  pthread_cond_t* signal=&(nodes_params->nodes_refreshed);

  pthread_mutex_lock(&mutex);
  nodes_params->pending_socket=new_socket_fd;

  int slot_ind;
  while ((slot_ind=getNewNodeIndex(nodes, nodes_params->max_nodes)) == -1){
    if (nodes_params->pending_socket == -1){ // no pending now
      printf("Pending socket closed\n");
      return;
    }
    printf("Waiting for empty slot\n");
    pthread_cond_wait(signal, &mutex);   
    printf("signal caught\n");
  }
  printf("Got slot %d\n", slot_ind);

  initNewNode(&(nodes[slot_ind]), 
	      nodes_params->unique_id_counter++, nodes_params->pending_socket);
  printf("Node %d: id=%d\n", slot_ind, nodes[slot_ind].id);

  nodes_params->pending_socket=-1;

  pthread_mutex_unlock(&mutex);
}

void kickNode(nodes_info* nodes_params, int id){
  node_data* nodes=nodes_params->nodes;
  pthread_cond_t* signal=&(nodes_params->nodes_refreshed);
  int max_nodes=nodes_params->max_nodes;
  pthread_mutex_t mutex=nodes_params->nodes_mutex;

  pthread_mutex_lock(&mutex);

  int index=getIndexById(nodes, max_nodes, id);

  close(nodes[index].socket_fd);

  pthread_join(nodes[index].send_thread, NULL);
  pthread_join(nodes[index].recv_thread, NULL);

  nodes[index].id=0;

  pthread_cond_signal(signal);
  pthread_mutex_unlock(&mutex);
  printf("Node %d kicked\n, now available %d\n", id, getNewNodeIndex(nodes, max_nodes));
}


void finalizeNodes(nodes_info* nodes_params){
  printf("started to finalize nodes\n");
  int max_nodes=nodes_params->max_nodes;
  int i;
  node_data* nodes=nodes_params->nodes;
  pthread_cond_t* signal=&(nodes_params->nodes_refreshed);
  pthread_mutex_t mutex=nodes_params->nodes_mutex;

  pthread_mutex_lock(&mutex);

  for (i=0; i<max_nodes; i++){
    if (nodes[i].id != 0){
      close(nodes[i].socket_fd);
    }
  }

  for (i=0; i<max_nodes; i++){
    if (nodes[i].id != 0){
      pthread_join(nodes[i].send_thread, NULL);
      pthread_join(nodes[i].recv_thread, NULL);
      nodes[i].id=0;
    }
  }
  printf("finalized nodes\n");
  pthread_cond_signal(signal);
  pthread_mutex_unlock(&mutex);
}

void closePendingConnections(nodes_info* nodes_params){
  pthread_cond_t* signal=&(nodes_params->nodes_refreshed);
  pthread_mutex_t mutex=nodes_params->nodes_mutex;

  pthread_mutex_lock(&mutex);

  close(nodes_params->pending_socket);
  nodes_params->pending_socket=-1;

  pthread_cond_signal(signal);
  pthread_mutex_unlock(&mutex);
}

int getNewNodeIndex(node_data* nodes, int count){
  int i;
  for (i=0; i<count; i++){
    if (nodes[i].id == 0){
      return i;
    }
  }
  return -1;
}

int getIndexById(node_data* nodes, int count, int id){
  int i;
  for (i=0; i<count; i++){
    if (nodes[i].id == id){
      return i;
    }
  }
  return -1;
}

void initNewNode(node_data* node, unsigned int id, unsigned int fd){
  node->id=id;
  node->socket_fd=fd;
  pthread_create(&(node->send_thread), NULL, 
		 &node_send_thread, (void*)(node));
  pthread_create(&(node->recv_thread), NULL, 
		 &node_recv_thread, (void*)(node));
}
