#include <pthread.h>
#include <stdio.h>

void threadLog(){
  pthread_t this_thread=pthread_self();
  unsigned int tid=(unsigned int)this_thread;
  printf("Thread %d:", tid);
}
