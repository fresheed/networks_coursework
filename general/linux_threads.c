#include "general/linux_threads.h"
#include <errno.h>
#include <stdio.h>

void runThread(u_thread* ut, void* thread_func(void* arg), void* with_arg){
  pthread_t* thread=&(ut->thread);
  pthread_create(thread, NULL,  thread_func, with_arg);
}

void createMutex(u_mutex* um){
  pthread_mutex_init(&(um->mutex), NULL);
}

void destroyMutex(u_mutex* um){
  pthread_mutex_destroy(&(um->mutex));
}

void lockMutex(u_mutex* um){
  pthread_mutex_lock(&(um->mutex));
}

void unlockMutex(u_mutex* um){
  pthread_mutex_unlock(&(um->mutex));
}

void createCondition(u_condition* um){
  pthread_cond_init(&(um->cond), NULL);
}

void destroyCondition(u_condition* uc){
  pthread_cond_destroy(&(uc->cond));
}

void blockOnCondition(u_condition* uc, u_mutex* um){
  pthread_cond_wait(&(uc->cond), &(um->mutex));
}

void blockWithTimeout(u_condition* uc, u_mutex* um, int timeout_ms){
  struct timespec start;
  gettimeofday(&start);
  struct timespec timeout_at=start;
  timeout_at.tv_sec+=(timeout_ms/1000);
  int result=pthread_cond_timedwait(&(uc->cond), &(um->mutex), &timeout_at);
  if ((result!=ETIMEDOUT) && (result!=0)){
      printf("Invalid cond_timedwait result: %d\n", result);
      perror("timedwait failed");
  }  
}


void signalAll(u_condition* uc){
  pthread_cond_broadcast(&(uc->cond));
}

void signalOne(u_condition* uc){
  pthread_cond_signal(&(uc->cond));
}

void waitForThread(u_thread* ut){
  pthread_join(ut->thread, NULL);
}
