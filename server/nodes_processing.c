//#include <pthread.h>
#include <stdio.h>
#include "server/nodes_processing.h"
#include "server/server_threads.h"
#include "general/common_threads.h"
#include "general/init_sockets.h"

void addNewNode(nodes_info* nodes_params, int new_socket_fd, primes_pool* pool){
  node_data* nodes=nodes_params->nodes;
  /* pthread_mutex_t mutex=nodes_params->nodes_mutex; */
  u_mutex* mutex=&(nodes_params->nodes_mutex);
  /* pthread_cond_t* signal=&(nodes_params->nodes_refreshed); */
  u_condition* signal=&(nodes_params->nodes_refreshed); 

  /* pthread_mutex_lock(&mutex); */
  lockMutex(mutex);
  nodes_params->pending_socket=new_socket_fd;

  int slot_ind;
  while ((slot_ind=getNewNodeIndex(nodes, nodes_params->max_nodes)) == -1){
    if (nodes_params->pending_socket == -1){ // no pending now
      printf("Pending socket closed\n");
      /* pthread_mutex_unlock(&mutex); */
      unlockMutex(mutex);
      return;
    }
    /* pthread_cond_wait(signal, &mutex); */
    blockOnCondition(signal, mutex);
  }
  printf("Got slot %d\n", slot_ind);

  initNewNode(&(nodes[slot_ind]), nodes_params,
	      nodes_params->unique_id_counter++,
	      nodes_params->pending_socket, pool);
  printf("Node %d: id=%d\n", slot_ind, nodes[slot_ind].id);

  nodes_params->pending_socket=-1;

  /* pthread_mutex_unlock(&mutex); */
  unlockMutex(mutex);
}

int assignTaskToNextNode(int last_executor, unsigned int lower_bound, unsigned int upper_bound, nodes_info* nodes_params){
  node_data* nodes=nodes_params->nodes;
  int max_nodes=nodes_params->max_nodes;
  /* pthread_mutex_t mutex=nodes_params->nodes_mutex; */
  u_mutex* mutex=&(nodes_params->nodes_mutex);

  /* pthread_mutex_lock(&mutex); */
  lockMutex(mutex);

  int index=last_executor, count=0;
  int id;
  while (count<max_nodes){
    printf("check for index %d: %d\n", index, nodes[index].id);
    if (nodes[index].id>0){ // node is active
      //id=nodes[index].id;
      break;
    }
    index=(index+1) % max_nodes;
    count++;
  }
  if (count == max_nodes){
    /* pthread_mutex_unlock(&mutex); */
    unlockMutex(mutex);    
    return -1;
  } else {
    message msg;
    fillGeneral(&msg, -1);
    createComputeRequest(&msg, -1, lower_bound, upper_bound);
    message* put_msg=putMessageInSet(msg, &(nodes[index].set), TO_SEND, 1);
  }

  /* pthread_mutex_unlock(&mutex); */
  unlockMutex(mutex);
  return (index+1) % max_nodes; // next time start from next executor
}

void kickSingleNode(nodes_info* nodes_params, int id){
  /* pthread_cond_t* signal=&(nodes_params->nodes_refreshed); */
  u_condition* signal=&(nodes_params->nodes_refreshed);
  /* pthread_mutex_t mutex=nodes_params->nodes_mutex; */
  u_mutex* mutex=&(nodes_params->nodes_mutex);

  /* pthread_mutex_lock(&mutex); */
  lockMutex(mutex);

  processKick(nodes_params, id);

  /* pthread_cond_signal(signal); */
  signalOne(signal);
  /* pthread_mutex_unlock(&mutex); */
  unlockMutex(mutex);
}


// NOT THREAD-SAFE, SHOULD BE GUARDED OUTSIDE !
void processKick(nodes_info* nodes_params, int id){
  node_data* nodes=nodes_params->nodes;
  int max_nodes=nodes_params->max_nodes;
  int index=getIndexById(nodes, max_nodes, id);
  printf("ind: %d\n", index);
  if (index == -1){
    printf("node not found\n");
    return; // no node found, maybe it has been already closed
  }

  message msg;
  fillGeneral(&msg, -1);
  createInitShutdownRequest(&msg, -1);
  message* put_msg=putMessageInSet(msg, &(nodes[index].set), TO_SEND, 1);
  // node receives message, calls shutdown(WR)
  // this server fails on read()
  // its recv thread call shutdown too and closes socket
  // so we only need to wait on recv completion
  
  /* pthread_join(nodes[index].recv_thread, NULL); */
  waitForThread(&(nodes[index].recv_thread));

  finalizeMessagesSet(&(nodes[index].set));

  nodes[index].id=0;
  printf("Node %d kicked\n", id);
}

void finalizeNodes(nodes_info* nodes_params){
  printf("started to finalize nodes\n");
  int max_nodes=nodes_params->max_nodes;
  int i;
  node_data* nodes=nodes_params->nodes;
  /* pthread_cond_t* signal=&(nodes_params->nodes_refreshed); */
  u_condition* signal=&(nodes_params->nodes_refreshed);
  /* pthread_mutex_t mutex=nodes_params->nodes_mutex; */
  u_mutex* mutex=&(nodes_params->nodes_mutex); 

  /* pthread_mutex_lock(&mutex); */
  lockMutex(mutex);

  for (i=0; i<max_nodes; i++){
    if (nodes[i].id != 0){
      processKick(nodes_params, nodes[i].id);
    }
  }

  printf("finalized nodes\n");
  /* pthread_cond_signal(signal); */
  signalOne(signal);
  /* pthread_mutex_unlock(&mutex); */
  unlockMutex(mutex);
}

void cleanupZombieNodes(nodes_info* nodes_params){
  u_mutex* mutex=&(nodes_params->nodes_mutex);
  node_data* nodes=nodes_params->nodes;
  lockMutex(mutex);

  int i;
  for (i=0; i<nodes_params->max_nodes; i++){
    if (nodes[i].id != 0 && (!nodes[i].set.is_active)){
      // node not killed but it not being processed
      processKick(nodes_params, nodes[i].id);
    }
  }

  unlockMutex(mutex);
}


void closePendingConnections(nodes_info* nodes_params, primes_pool* pool){
  /* pthread_cond_t* signal=&(nodes_params->nodes_refreshed); */
  u_condition* signal=&(nodes_params->nodes_refreshed); 
  /* pthread_mutex_t mutex=nodes_params->nodes_mutex; */
  u_mutex* mutex=&(nodes_params->nodes_mutex);

  /* pthread_mutex_lock(&mutex); */
  lockMutex(mutex);
  int pending=nodes_params->pending_socket;
  if (pending < 0) {
    /* pthread_mutex_unlock(&mutex); */
    printf("No pending connections found\n");
    unlockMutex(mutex);
    return;
  }
  printf("Temporary creating and closing pending...\n");
  node_data temp_node;
  int temp_id=10;
  initNewNode(&temp_node, NULL, temp_id, pending, NULL);
  
  message msg;
  fillGeneral(&msg, -1);
  createInitShutdownRequest(&msg, -1);
  message* put_msg=putMessageInSet(msg, &(temp_node.set), TO_SEND, 1);
  waitForThread(&(temp_node.recv_thread));
  finalizeMessagesSet(&(temp_node.set));

  /* close(nodes_params->pending_socket); */
  nodes_params->pending_socket=-1;

  /* pthread_cond_signal(signal); */
  signalOne(signal);
  /* pthread_mutex_unlock(&mutex); */
  unlockMutex(mutex);
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

void initNewNode(node_data* node, nodes_info* nodes_params, unsigned int id, unsigned int fd, primes_pool* pool){
  node->id=id;
  node->socket_fd=fd;

  initMessagesSet(&(node->set));

  node->common_pool=pool;

  /* pthread_create(&(node->send_thread), NULL, &common_send_thread, (void*)(node)); */
  runThread(&(node->send_thread), &common_send_thread, (void*)(node));
  /* pthread_create(&(node->proc_thread), NULL, &server_proc_thread, (void*)(node)); */
  runThread(&(node->proc_thread), &server_proc_thread, (void*)(node));
  /* pthread_create(&(node->recv_thread), NULL, &common_recv_thread, (void*)(node)); */
  runThread(&(node->recv_thread), &common_recv_thread, (void*)(node));
}

void printNodes(nodes_info* nodes_params){
  u_mutex* mutex=&(nodes_params->nodes_mutex);
  node_data* nodes=nodes_params->nodes;
  lockMutex(mutex);
  printf("Connected nodes' IDs:\n");
  int i;
  for (i=0; i<nodes_params->max_nodes; i++){
    if (nodes[i].id != 0){
      printf("%d \n", nodes[i].id);
    }
  }
  printf("Pending: %d\n", (nodes_params->pending_socket==-1)?0:1);

  unlockMutex(mutex);
}
