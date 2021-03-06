//#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "general/messages.h"

void createRequest(message* msg, unsigned char known_id){
  fillGeneral(msg, known_id);
  msg->status_type=REQUEST;
}

void createResponse(message* msg, unsigned char known_id, unsigned char response_to){
  fillGeneral(msg, known_id);
  msg->status_type=RESPONSE;
  msg->response_to=response_to;
}

int createMaxRequest(message* msg, unsigned char known_id){
  createRequest(msg, known_id);
  msg->info_type=MAX_INFO;
}

int createMaxResponse(message* msg, unsigned char known_id, unsigned char response_to, long value){
  createResponse(msg, known_id, response_to);
  msg->info_type=MAX_INFO;
  //msg->data_len=value; // temp solution, should be passed via data
  char data_buffer[LIMIT_DATA_LEN];
  memset(data_buffer, 0, LIMIT_DATA_LEN);
  long ivalue=value;
  writeNumsToChars(&ivalue, 1, data_buffer);
  addData(msg, data_buffer, strlen(data_buffer));
}

int createComputeRequest(message* msg, unsigned char known_id, long lower_bound, long upper_bound){
  createRequest(msg, known_id);
  msg->info_type=COMPUTE_INFO;
  char data_buffer[LIMIT_DATA_LEN];
  memset(data_buffer, 0, LIMIT_DATA_LEN);
  sprintf(data_buffer, "%ld %ld", lower_bound, upper_bound);
  addData(msg, data_buffer, strlen(data_buffer));
}

int createComputeResponse(message* msg, unsigned char known_id, unsigned char response_to, primes_range* range){
  createResponse(msg, known_id, response_to);
  msg->info_type=COMPUTE_INFO;
  printf("LDL: %ld\n", (long)LIMIT_DATA_LEN);
  //char data_buffer[LIMIT_DATA_LEN];
  char* data_buffer=(char*)malloc(LIMIT_DATA_LEN*sizeof(char));
  memset(data_buffer, 0, LIMIT_DATA_LEN);
  char* write_ptr=data_buffer;

  long primes_amount=getPrimesCountInRange(range);
  long appended=writeNumsToChars(&primes_amount, 1, write_ptr);
  write_ptr+=appended;
  long header[]={range->lower_bound, range->upper_bound};
  appended=writeNumsToChars(header, 2, write_ptr);
  write_ptr+=appended;
  printf("Writing...\n");
  appended=writeNumsToChars(range->numbers, primes_amount, write_ptr);
  printf("Appended: %ld\n", appended);
  if (appended < 0){
    printf("Buffer size exceeded but IGNORED!\n");
    msg->is_ok=0;
    msg->data=NULL;
    msg->data_len=0;
    free(data_buffer);
    return;
  }

  addData(msg, data_buffer, strlen(data_buffer));
  free(data_buffer);
}

int createRecentRequest(message* msg, unsigned char known_id, long amount){
  createRequest(msg, known_id);
  msg->info_type=RECENT_INFO;
  char data_buffer[LIMIT_DATA_LEN];
  memset(data_buffer, 0, LIMIT_DATA_LEN);
  sprintf(data_buffer, "%ld ", amount);
  addData(msg, data_buffer, strlen(data_buffer));
}

int createRecentResponse(message* msg, unsigned char known_id, unsigned char response_to, long* nums, long amount){
  createResponse(msg, known_id, response_to);
  msg->info_type=RECENT_INFO;
  char data_buffer[LIMIT_DATA_LEN];
  memset(data_buffer, 0, LIMIT_DATA_LEN);
  char* write_ptr=data_buffer;

  int appended=writeNumsToChars(&amount, 1, write_ptr);
  write_ptr+=appended;
  appended=writeNumsToChars(nums, amount, write_ptr);
  if (appended < 0){
    printf("Buffer size exceeded but IGNORED!\n");
    msg->is_ok=0;
    msg->data=NULL;
    msg->data_len=0;
    return;
  }

  addData(msg, data_buffer, strlen(data_buffer));
}

int createInitShutdownRequest(message* msg, unsigned char known_id, int amount){
  createRequest(msg, known_id);
  msg->info_type=INIT_SHUTDOWN;
  return 1;
}


long writeNumsToChars(long* nums, long amount, char* raw){
  long i;
  long appended;
  char* write_ptr=raw;
  for (i=0; i<amount; i++){
    appended=sprintf(write_ptr, "%ld ", nums[i]);
    write_ptr+=appended;
    if (write_ptr-raw > LIMIT_DATA_LEN){
      printf("Tried to write over %d, stopping\n", LIMIT_DATA_LEN);
      return -1;
    }
  }
  return (write_ptr-raw);
}

long readNumsFromChars(char* raw, long* nums, long amount){
  long i;
  long appended;
  char* write_ptr=raw;
  for (i=0; i<amount; i++){
    // %n to determine amount of bytes read
    sscanf(write_ptr, "%ld %ln", &(nums[i]), &appended);
    write_ptr+=appended;
  }
  return (write_ptr-raw);
}


void fillGeneral(message* msg, unsigned char known_id){
  msg->data=NULL;
  msg->data_len=0;
  msg->is_ok=1;
  if (known_id >= 0){
    msg->internal_id=known_id;
  } else {
    msg->internal_id=-1;
  }
}

void addData(message* msg, char* new_data, unsigned int len){

  msg->data=(char*)malloc((len+1)*sizeof(char));
  memset(msg->data, 0, len+1);
  memcpy(msg->data, new_data, len);
  msg->data_len=len;
}


message* putMessageInSet(message msg, messages_set* set, char new_status, int generate_id){
  // message will be COPIED into set
  // returns pointer to message in set
  u_mutex* mutex=&(set->messages_mutex);
  u_condition* was_changed=&(set->status_changed);

  lockMutex(mutex);
  if (generate_id){
    msg.internal_id=(set->next_id)++;
  }

  message* slot_ptr=NULL;
  while ( (slot_ptr=findMessageWithStatus(set, EMPTY_SLOT)) == NULL){
    blockOnCondition(was_changed, mutex);
    if (!(set->is_active)){
      unlockMutex(mutex);
      return NULL;
    }
  }

  *slot_ptr=msg;
  slot_ptr->current_status=new_status;
  signalAll(was_changed);

  unlockMutex(mutex);

  return slot_ptr;
}

message* lockNextMessage(messages_set* set, char cur_status){
  u_mutex* mutex=&(set->messages_mutex);
  u_condition* was_changed=&(set->status_changed);

  lockMutex(mutex);

  message* slot_ptr=NULL;
  while ( (slot_ptr=findMessageWithStatus(set, cur_status)) == NULL){
    blockOnCondition(was_changed, mutex);
    if (!(set->is_active)){
      unlockMutex(mutex);
      return NULL;
    }
  }

  slot_ptr->current_status=OWNED;

  unlockMutex(mutex);

  return slot_ptr;

}

message* findMessageWithStatus(messages_set* set, char status){
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

message* findMessageById(messages_set* set, char id){
  u_mutex* mutex=&(set->messages_mutex);
  lockMutex(mutex);

  int i;
  for (i=0; i<MESSAGES_SET_SIZE; i++){
    if (set->messages[i].internal_id == id){
      unlockMutex(mutex);
      return &(set->messages[i]);
    }
  }
  unlockMutex(mutex);
  return NULL;
}

void updateMessageStatus(message* msg, messages_set* set, char new_status){
  u_mutex* mutex=&(set->messages_mutex);
  u_condition* change=&(set->status_changed);
  lockMutex(mutex);

  msg->current_status=new_status;
  if (new_status == EMPTY_SLOT) {
    finalizeMessage(msg);
  }

  signalAll(change);
  unlockMutex(mutex);
}

/* void changeStatus(, messages_set* set){ */


/* } */

void initMessagesSet(messages_set* set){
  int i;
  for (i=0; i<MESSAGES_SET_SIZE; i++){
    (set->messages)[i].current_status=EMPTY_SLOT;
    (set->messages)[i].data=NULL;
  }
  set->to_put=0;
  set->to_process=0;
  set->to_send=0;
  set->is_active=1;
  createMutex(&(set->messages_mutex));
  createCondition(&(set->status_changed));
}


void printMessage(message* msg){
  printf("%ld, %ld\n", sizeof(int), sizeof(long));
  printf("IntID: %d, ST: %d, IT: %d, RT: %d, CS: %d, DL: %d\n",
	 msg->internal_id, msg->status_type, msg->info_type,
	 msg->response_to, msg->current_status, msg->data_len);
  if (msg->data != NULL){
    printf("Addata: %s\n", msg->data);
  }
}

void finalizeMessagesSet(messages_set* set){
  int i;
  for (i=0; i<MESSAGES_SET_SIZE; i++){
    (set->messages)[i].current_status=EMPTY_SLOT;
    finalizeMessage(&(set->messages[i]));
  }
  destroyMutex(&(set->messages_mutex));
  destroyCondition(&(set->status_changed));
}

void markSetInactive(messages_set* set){
  u_mutex* mutex=&(set->messages_mutex);
  u_condition* change=&(set->status_changed);
  lockMutex(mutex);

  set->is_active=0;
  signalAll(change);

  unlockMutex(mutex);
  printf("Marked set as inactive..\n");
}

void finalizeMessage(message* msg){
  if (msg->data != NULL){
    free(msg->data);
    msg->data=NULL;
  }
}
