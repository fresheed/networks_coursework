#include <pthread.h>

#ifndef messages_h
#define messages_h

#define REQUEST 10
#define RESPONSE 1

#define MAX_INFO 10
#define RANGE_INFO 1
#define COMP_INFO 2

#define EMPTY_SLOT 10
#define TO_PROCESS 1
#define TO_SEND 2
#define WAITS_RESPONSE 3
#define OWNED 4

#define MESSAGES_SET_SIZE 10

typedef struct message {
  char internal_id;
  char status_type;
  char info_type;
  char response_to;
  char current_status;
  char data_len;
  char* data;
} message;

typedef struct messages_set {
  char next_id;
  unsigned int is_active;
  message messages[MESSAGES_SET_SIZE];
  unsigned int to_send, to_put, to_process;
  pthread_mutex_t messages_mutex;
  //  pthread_cond_t new_empty_slot;
  pthread_cond_t status_changed;
} messages_set;
 
#endif

void createRequest(message* msg, unsigned char known_id);
void createResponse(message* msg, unsigned char known_id, unsigned char response_to);

void createMaxRequest(message* msg, unsigned char known_id);
void createMaxResponse(message* msg, unsigned char known_id, unsigned char response_to, char value);
void createRangeRequest(message* msg, unsigned char known_id, int lower_bound, int upper_bound);
void createRangeResponse(message* msg, unsigned char known_id, unsigned char response_to, int* primes, int primes_amount);

int writeNumsToChars(int* nums, int amount, char* raw);
int readNumsFromChars(char* raw, int* nums, int amount);

void fillGeneral(message* msg, unsigned char known_id);
void addData(message* msg, char* data, unsigned int len);
message* putMessageInSet(message message, messages_set* set, char new_status, int generate_id);
message* lockNextMessage(messages_set* set, char cur_status);
message* findMessageWithStatus(messages_set* set, char status);
message* findMessageById(messages_set* set, char id);
void updateMessageStatus(message* msg, messages_set* set, char new_status);
void initMessagesSet(messages_set* set);
void markSetInactive(messages_set* set);
void printMessage(message* msg);
void finalizeMessageSet(messages_set* set);
void finalizeMessage();

