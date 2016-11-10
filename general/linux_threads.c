#include "general/linux_threads.h"

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

void signalAll(u_condition* uc){
  pthread_cond_broadcast(&(uc->cond));
}

void signalOne(u_condition* uc){
  pthread_cond_signal(&(uc->cond));
}

void waitForThread(u_thread* ut){
  pthread_join(ut->thread, NULL);
}
