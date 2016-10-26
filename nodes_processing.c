#include <pthread.h>
#include <stdio.h>
#include "nodes_processing.h"
#include "server_threads.h"


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
    pthread_cond_wait(signal, &mutex);   
  }
  printf("Got slot %d\n", slot_ind);

  initNewNode(&(nodes[slot_ind]), nodes_params,
	      nodes_params->unique_id_counter++, nodes_params->pending_socket);
  printf("Node %d: id=%d\n", slot_ind, nodes[slot_ind].id);

  nodes_params->pending_socket=-1;

  pthread_mutex_unlock(&mutex);
}

void kickNode(nodes_info* nodes_params, int id){
  printf("Kick entered\n");
  node_data* nodes=nodes_params->nodes;
  pthread_cond_t* signal=&(nodes_params->nodes_refreshed);
  int max_nodes=nodes_params->max_nodes;
  pthread_mutex_t mutex=nodes_params->nodes_mutex;

  pthread_mutex_lock(&mutex);
  printf("Kick lock\n");
  int index=getIndexById(nodes, max_nodes, id);
  if (index == -1){
    printf("node not found\n");
    return; // no node found, maybe it has been already closed
  }
  printf("node %d\n", index);

  close(nodes[index].socket_fd);

  pthread_join(nodes[index].send_thread, NULL);
  pthread_join(nodes[index].recv_thread, NULL);
  pthread_join(nodes[index].proc_thread, NULL);
  finalizeMessagesSet(&(nodes[index].set));

  nodes[index].id=0;

  pthread_cond_signal(signal);
  pthread_mutex_unlock(&mutex);
  printf("Node %d kicked, now available %d\n", id, getNewNodeIndex(nodes, max_nodes));
}


void finalizeNodes(nodes_info* nodes_params){
  printf("started to finalize nodes\n");
  int max_nodes=nodes_params->max_nodes;
  int i;
  node_data* nodes=nodes_params->nodes;
  pthread_cond_t* signal=&(nodes_params->nodes_refreshed);
  pthread_mutex_t mutex=nodes_params->nodes_mutex;

  pthread_mutex_lock(&mutex);

  printf("closing\n");
  for (i=0; i<max_nodes; i++){
    if (nodes[i].id != 0){
      close(nodes[i].socket_fd);
    }
  }

  printf("joining\n");
  for (i=0; i<max_nodes; i++){
    if (nodes[i].id != 0){
      printf("join 0\n");
      pthread_join(nodes[i].send_thread, NULL);
      printf("join 1\n");
      pthread_join(nodes[i].recv_thread, NULL);
      printf("join 2\n");
      pthread_join(nodes[i].proc_thread, NULL);
      printf("join 3\n");
      finalizeMessagesSet(&(nodes[i].set));
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

void closeDisconnectedNodes(nodes_info* nodes_params){
  int i;
  node_data* nodes=nodes_params->nodes;
  for (i=0; i<nodes_params->max_nodes; i++){
    if (nodes[i].id != 0 && (!nodes[i].set.is_active)){
      int orig_id=nodes[i].id;
      kickNode(nodes_params, orig_id);
      printf("detected disconnect of node %d, closed\n", orig_id);
    }
  }

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
  printf("looking for %d\n", id);
  int i;
  for (i=0; i<count; i++){
    if (nodes[i].id == id){
      return i;
    }
  }
  return -1;
}

void initNewNode(node_data* node, nodes_info* nodes_params, unsigned int id, unsigned int fd){
  node->id=id;
  node->socket_fd=fd;

  initMessagesSet(&(node->set));
  
  node->nodes_params=(void*)nodes_params;
  
  pthread_create(&(node->send_thread), NULL, 
		 &server_send_thread, (void*)(node));
  pthread_create(&(node->recv_thread), NULL, 
		 &server_recv_thread, (void*)(node));
  pthread_create(&(node->proc_thread), NULL, 
		 &server_proc_thread, (void*)(node));
}
