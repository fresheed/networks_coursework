#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "general/messages.h"

void createRequest(message* msg, unsigned int known_id, unsigned short source_type){
  fillGeneral(msg, known_id, source_type);
}

void createResponse(message* msg, unsigned int known_id, unsigned short source_type, unsigned int response_to){
  fillGeneral(msg, known_id, source_type);
  msg->response_to=response_to;
}

void fillGeneral(message* msg, unsigned int known_id, unsigned short source_type){
  msg->data=NULL;
  if (known_id >= 0){
    msg->internal_id=known_id;
  } else {
    msg->internal_id=-1;
  }
  msg->source_type=source_type;
  msg->status_type=RESPONSE;
}

void addData(message* msg, char* new_data, unsigned int len){
  msg->data=(char*)malloc(len*sizeof(char));
  memcpy(msg->data, new_data, len);
  msg->data_len=len;
}


message* putMessageInSet(message msg, messages_set* set, int new_status){
  // message will be COPIED into set
  // returns pointer to message in set
  pthread_mutex_t* mutex=&(set->messages_mutex);
  pthread_cond_t* was_changed=&(set->status_changed);
  
  pthread_mutex_lock(mutex);
  if (msg.internal_id<0){
    msg.internal_id=set->next_id++;
  }

  message* slot_ptr=NULL;
  while ( (slot_ptr=findMessageWithStatus(set, EMPTY_SLOT)) == NULL){
    pthread_cond_wait(was_changed, mutex);
    if (!(set->is_active)){
      pthread_mutex_unlock(mutex);
      return NULL;
    }
  }

  *slot_ptr=msg;
  slot_ptr->current_status=new_status;
  pthread_cond_broadcast(was_changed);

  pthread_mutex_unlock(mutex);

  return slot_ptr;
}

message* lockNextMessage(messages_set* set, int cur_status){
  pthread_mutex_t* mutex=&(set->messages_mutex);
  pthread_cond_t* was_changed=&(set->status_changed);
  
  pthread_mutex_lock(mutex);

  message* slot_ptr=NULL;
  while ( (slot_ptr=findMessageWithStatus(set, cur_status)) == NULL){
    pthread_cond_wait(was_changed, mutex);
    if (!(set->is_active)){
      pthread_mutex_unlock(mutex);
      return NULL;
    }
  }

  slot_ptr->current_status=OWNED;

  pthread_mutex_unlock(mutex);

  return slot_ptr;

}

message* findMessageWithStatus(messages_set* set, int status){
  int* init_pos, cnt, pos;
  
  switch(status){
  case EMPTY_SLOT:
    init_pos=&(set->to_put); break;
  case TO_PROCESS:
    init_pos=&(set->to_process); break;
  case TO_SEND:
    init_pos=&(set->to_send); break;    
  }
  cnt=0;
  pos=*init_pos;
  message* result=NULL;
  while (cnt<MESSAGES_SET_SIZE){
    //printf("checking %d\n", pos);
    if (set->messages[pos].current_status == status){
      result=&((set->messages)[pos]);
      break;
    }
    pos=(++pos)%MESSAGES_SET_SIZE;
    cnt++;
  }
  //printf("checked slots from %d, returning slot %d\n", *init_pos, pos);
  if (result != NULL){
    *init_pos=((*init_pos)+1)%MESSAGES_SET_SIZE;
  }
  return result;
}

void updateMessageStatus(message* msg, messages_set* set, int new_status){
  pthread_mutex_t* mutex=&(set->messages_mutex);
  pthread_cond_t* change=&(set->status_changed);
  pthread_mutex_lock(mutex);
  
  msg->current_status=new_status;

  pthread_cond_broadcast(change);
  pthread_mutex_unlock(mutex);
}

/* void changeStatus(, messages_set* set){ */
  

/* } */

void initMessagesSet(messages_set* set){
  int i;
  for (i=0; i<MESSAGES_SET_SIZE; i++){
    (set->messages)[i].current_status=EMPTY_SLOT;
  }
  set->to_put=0;
  set->to_process=0;
  set->to_send=0;
  set->is_active=1;
  pthread_mutex_init(&(set->messages_mutex), NULL);
  pthread_cond_init(&(set->status_changed), NULL);
}

void finalizeMessagesSet(messages_set* set){
  int i;
  for (i=0; i<MESSAGES_SET_SIZE; i++){
    (set->messages)[i].current_status=EMPTY_SLOT;
    finalizeMessage(&(set->messages[i]));
  }
  pthread_mutex_destroy(&(set->messages_mutex));
  pthread_cond_destroy(&(set->status_changed));
}

void markSetInactive(messages_set* set){
  printf("Marking as inactive..\n");
  pthread_mutex_t* mutex=&(set->messages_mutex);
  pthread_cond_t* change=&(set->status_changed);
  pthread_mutex_lock(mutex);

  set->is_active=0;
  pthread_cond_broadcast(change);

  pthread_mutex_unlock(mutex);
  printf("Marked..\n");

}

void finalizeMessage(message* msg){
  if (msg->data != NULL){
    free(msg->data);
    msg->data=NULL;
  }
}
