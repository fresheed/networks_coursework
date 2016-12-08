//#include <pthread.h>
#include "general/u_threads.h"
#include "general/logic.h"

#ifndef messages_h
#define messages_h

#define REQUEST 10
#define RESPONSE 1
#define ACK 2

#define MAX_INFO 10
#define COMPUTE_INFO 2
#define RECENT_INFO 3
#define INIT_SHUTDOWN 4
//#define REGISTER_CLIENT 5

#define EMPTY_SLOT 10
#define TO_PROCESS 1
#define TO_SEND 2
#define WAITS_RESPONSE 3
#define OWNED 4

#define MESSAGES_SET_SIZE 20
#define CHARS_FOR_DATA_LEN_FIELD 6
#define LIMIT_DATA_LEN 200000
// id + req/resp + type + resp_to + is_ok + data_len
#define HEADER_LEN (1+1+1+1+1+CHARS_FOR_DATA_LEN_FIELD)

typedef struct {
  char cur_recv_id;
  char cur_send_id;  
  int was_acknowledged;
  u_mutex ack_mutex;
  u_condition updated_ack_status;
} udp_integrity;

typedef struct message {
  char internal_id;
  char status_type;
  char info_type;
  char response_to;
  char is_ok;
  char current_status;
  unsigned int data_len;
  char* data;
} message;

typedef struct messages_set {
  char next_id;
  unsigned int is_active;
  message messages[MESSAGES_SET_SIZE];
  unsigned int to_send, to_put, to_process;
  u_mutex messages_mutex;
  u_condition status_changed;
#ifdef UDP_TRANSFER
  udp_integrity integrity;
#endif
} messages_set;

#endif

void createRequest(message* msg, unsigned char known_id);
void createResponse(message* msg, unsigned char known_id, unsigned char response_to);

int createMaxRequest(message* msg, unsigned char known_id);
int createMaxResponse(message* msg, unsigned char known_id, unsigned char response_to, long value);
int createComputeRequest(message* msg, unsigned char known_id, long lower_bound, long upper_bound);
int createComputeResponse(message* msg, unsigned char known_id, unsigned char response_to, primes_range* range);
int createRecentRequest(message* msg, unsigned char known_id, long amount);
int createRecentResponse(message* msg, unsigned char known_id, unsigned char response_to, long* nums, long amount);
int createInitShutdownRequest(message* msg, unsigned char known_id);
//int createRegisterClientRequest(message* msg, unsigned char known_id, int amount);

long writeNumsToChars(long* nums, long amount, char* raw);
long readNumsFromChars(char* raw, long* nums, long amount);

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
void finalizeMessagesSet(messages_set* set);
void finalizeMessage();


