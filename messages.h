#include <pthread.h>

#ifndef messages_h
#define messages_h

#define INCOMING 0
#define OUTGOING 1

#define REQUEST 0
#define RESPONSE 1

#define EMPTY_SLOT 0
#define TO_PROCESS 1
#define TO_SEND 2
#define WAITS_RESPONSE 3
#define OWNED

typedef struct message {
  unsigned int id;
  unsigned short source_type; 
  unsigned short status_type;
  unsigned int response_to;
  unsigned short current_status;
  unsigned int data_len;
  char* data;
} message;

typedef struct messages_set {
  unsigned int set_size;
  unsigned int next_id;
  message* messages;
  unsigned int to_send, to_put, to_process;
  pthread_mutex_t messages_mutex;
} messages_set;
 


#endif
