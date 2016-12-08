#include <pthread.h>


#ifndef u_linux_threads
#define u_linux_threads

typedef struct u_thread {
  pthread_t thread;
} u_thread;

typedef struct u_mutex {
  pthread_mutex_t mutex;
} u_mutex;

typedef struct u_condition {
  pthread_cond_t cond;
} u_condition;

#endif // u_linux_threads


void runThread(u_thread* ut, void* thread_func(void* arg), void* with_arg);
void createMutex(u_mutex* um);
void destroyMutex(u_mutex* um);
void lockMutex(u_mutex* um);
void unlockMutex(u_mutex* um);
void createCondition(u_condition* um);
void destroyCondition(u_condition* uc);
void blockOnCondition(u_condition* uc, u_mutex* um);
void blockWithTimeout(u_condition* uc, u_mutex* um, int timeout_ms);
void signalAll(u_condition* uc);
void signalOne(u_condition* uc);
void waitForThread(u_thread* ut);
