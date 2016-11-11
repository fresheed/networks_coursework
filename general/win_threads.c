#include "general/win_threads.h"

void runThread(u_thread* ut, void* thread_func(void* arg), void* with_arg){
//  pthread_t* thread=&(ut->thread);
//  pthread_create(thread, NULL,  thread_func, with_arg);
  ut->thread=_beginthread(thread_func, 0, with_arg);
}

void createMutex(u_mutex* um){
//  pthread_mutex_init(&(um->mutex), NULL);
    um->mutex = CreateMutex(
                NULL,             // default security attributes
                FALSE,             // initially not owned
                NULL);             // unnamed mutex
}

void destroyMutex(u_mutex* um){
//  pthread_mutex_destroy(&(um->mutex));
    CloseHandle(um->mutex);
}

void lockMutex(u_mutex* um){
//  pthread_mutex_lock(&(um->mutex));
    WaitForSingleObject(um->mutex, INFINITE);  // no time-out interval
}


void unlockMutex(u_mutex* um){
//  pthread_mutex_unlock(&(um->mutex));
    ReleaseMutex(um->mutex);
}

void createCondition(u_condition* um){
    um->cond = CreateEvent(
            NULL,               // default security attributes
            TRUE,               // 1 - manual, 0 - auto
            FALSE,              // initial state is nonsignaled
            NULL    // object name
            );
}

void destroyCondition(u_condition* uc){
//  pthread_cond_destroy(&(uc->cond));
    CloseHandle(uc->cond);
}

void blockOnCondition(u_condition* uc, u_mutex* um){
//  pthread_cond_wait(&(uc->cond), &(um->mutex));
    SignalObjectAndWait(um->mutex, uc->cond, INFINITE, FALSE);
    WaitForSingleObject(um->mutex, INFINITE);  // no time-out interval
}

void signalAll(u_condition* uc){
//  pthread_cond_broadcast(&(uc->cond));
    //SetEvent(uc->cond);
    PulseEvent(uc->cond);
}

void signalOne(u_condition* uc){
//  pthread_cond_signal(&(uc->cond));
   // SetEvent(uc->cond);
    PulseEvent(uc->cond);
}

void waitForThread(u_thread* ut){
//  pthread_join(ut->thread, NULL);
    WaitForSingleObject(ut->thread,INFINITE);
    }

