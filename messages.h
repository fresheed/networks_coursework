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
#define OWNED 4

#define MESSAGES_SET_SIZE 10

typedef struct message {
  unsigned int internal_id;
  unsigned short source_type; 
  unsigned short status_type;
  unsigned int response_to;
  unsigned short current_status;
  unsigned int data_len;
  char* data;
} message;

typedef struct messages_set {
  unsigned int next_id;
  unsigned int is_active;
  message messages[MESSAGES_SET_SIZE];
  unsigned int to_send, to_put, to_process;
  pthread_mutex_t messages_mutex;
  //  pthread_cond_t new_empty_slot;
  pthread_cond_t status_changed;
} messages_set;
 
#endif

void createRequest(message* msg, unsigned int known_id, unsigned short source_type);

void createResponse(message* msg, unsigned int known_id, unsigned short source_type, unsigned int response_to);

void fillGeneral(message* msg, unsigned int known_id, unsigned short source_type);

void addData(message* msg, char* data, unsigned int len);

message* putMessageInSet(message message, messages_set* set, int new_status);

message* lockNextMessage(messages_set* set, int cur_status);

message* findMessageWithStatus(messages_set* set, int status);

void updateMessageStatus(message* msg, messages_set* set, int new_status);

void initMessagesSet(messages_set* set);

void markSetInactive(messages_set* set);

void finalizeMessageSet(messages_set* set);

void finalizeMessage();

