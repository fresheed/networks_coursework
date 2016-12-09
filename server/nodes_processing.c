#include <stdio.h>
#include "server/nodes_processing.h"
#include "server/server_threads.h"
#include "general/common_threads.h"
#include "general/init_sockets.h"

void addNewNode(nodes_info* nodes_params, socket_conn new_conn, primes_pool* pool){
  node_data* nodes=nodes_params->nodes;
  u_mutex* mutex=&(nodes_params->nodes_mutex);
  u_condition* signal=&(nodes_params->nodes_refreshed); 

  lockMutex(mutex);
  
  nodes_params->pending_conn=new_conn;

  int slot_ind;
  while ((slot_ind=getNewNodeIndex(nodes, nodes_params->max_nodes)) == -1){
    if (nodes_params->pending_conn.socket_fd == -1){ // no pending now
      printf("Pending socket closed\n");
      unlockMutex(mutex);
      return;
    }
    blockOnCondition(signal, mutex);
  }

  initNewNode(&(nodes[slot_ind]), nodes_params,
	      nodes_params->unique_id_counter++,
	      nodes_params->pending_conn, pool);
  printf("New node %d: id=%d\n", slot_ind, nodes[slot_ind].id);

  nodes_params->pending_conn=err_socket;

  unlockMutex(mutex);
}

int assignTaskToNextNode(int last_executor, long lower_bound, long upper_bound, nodes_info* nodes_params){
  node_data* nodes=nodes_params->nodes;
  int max_nodes=nodes_params->max_nodes;
  u_mutex* mutex=&(nodes_params->nodes_mutex);

  lockMutex(mutex);

  int index=last_executor, count=0;
  int id;
  while (count<max_nodes){
    if (nodes[index].id>0){ // node is active
      //id=nodes[index].id;
      break;
    }
    index=(index+1) % max_nodes;
    count++;
  }
  if (count == max_nodes){
    unlockMutex(mutex);    
    return -1;
  } else {
    message msg;
    fillGeneral(&msg, -1);
    createComputeRequest(&msg, -1, lower_bound, upper_bound);
    message* put_msg=putMessageInSet(msg, &(nodes[index].set), TO_SEND, 1);
  }

  unlockMutex(mutex);
  return (index+1) % max_nodes; // next time start from next executor
}

void kickSingleNode(nodes_info* nodes_params, int id){
  u_condition* signal=&(nodes_params->nodes_refreshed);
  u_mutex* mutex=&(nodes_params->nodes_mutex);

  lockMutex(mutex);

  processKick(nodes_params, id);

  signalOne(signal);
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
  waitForThread(&(nodes[index].recv_thread));
  socketClose(nodes[index].conn.pipe_in_fd);
  
  finalizeMessagesSet(&(nodes[index].set));

  // pipes should be closed already in recv thread

  nodes[index].id=0;
  printf("Node %d kicked\n", id);
}

void finalizeNodes(nodes_info* nodes_params){
  printf("started to finalize nodes\n");
  int max_nodes=nodes_params->max_nodes;
  int i;
  node_data* nodes=nodes_params->nodes;
  u_condition* signal=&(nodes_params->nodes_refreshed);
  u_mutex* mutex=&(nodes_params->nodes_mutex); 

  lockMutex(mutex);

  for (i=0; i<max_nodes; i++){
    if (nodes[i].id != 0){
      processKick(nodes_params, nodes[i].id);
    }
  }

  printf("finalized nodes\n");
  signalOne(signal);
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

int getIndexByAddress(node_data* nodes, int count, struct sockaddr_in address){
  int i;
  for (i=0; i<count; i++){
    struct sockaddr_in cur_address=nodes[i].conn.peer_address;
    if (addressesAreEqual(address, cur_address)){
      return i;
    }
  }
  return -1;
}

void initNewNode(node_data* node, nodes_info* nodes_params, unsigned int id, socket_conn conn, primes_pool* pool){
  node->id=id;
  node->conn=conn;

  initMessagesSet(&(node->set));

  node->common_pool=pool;

  runThread(&(node->send_thread), &common_send_thread, (void*)(node));
  runThread(&(node->proc_thread), &server_proc_thread, (void*)(node));
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
  printf("Pending: %d\n", (nodes_params->pending_conn.socket_fd==-1)?0:1);

  unlockMutex(mutex);
}
