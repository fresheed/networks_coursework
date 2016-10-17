void createRequest(message* msg, unsigned short source_type){
  msg->data=NULL;
  msg->source_type=source_type;
  msg->status_type=REQUEST;
}

void createResponse(message* msg, unsigned short source_type, unsigned int response_to){
  msg->data=NULL;
  msg->source_type=source_type;
  msg->status_type=RESPONSE;
  msg->response_to=response_to;
}

void addData(message* msg, char* data, unsigned int len){
  msg->data=malloc(fasdfafafd);
  memcpy( fadfafda);
  msg->data_len=len;
}


int putMessageInSet(message message, messages_set* set){
  // message will be COPIED into set
  // returns id
  pthread_mutex_t* mutex=&(set->messages_mutex);
  
  pthread_mutex_lock(mutex);

  slot_ptr=find_next_free_slot();
  

  pthread_mutex_unlock(mutex);

  return id;
}

void changeStatus(int id, messages_set* set){
  

}

void finalizeMessage(){
  if (msg->data != NULL){
    free(msg->data);

  }
}
