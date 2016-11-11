#include <windows.h>
#include <process.h>

#ifndef u_win_threads
#define u_win_threads

typedef struct u_thread {
    HANDLE thread;
} u_thread;

typedef struct u_mutex {
    HANDLE mutex;
} u_mutex;

typedef struct u_condition {
    HANDLE cond;
} u_condition;

#endif // u_win_threads

void runThread(u_thread* ut, void* thread_func(void* arg), void* with_arg);
void createMutex(u_mutex* um);
void destroyMutex(u_mutex* um);
void lockMutex(u_mutex* um);
void unlockMutex(u_mutex* um);
void createCondition(u_condition* um);
void destroyCondition(u_condition* uc);
void blockOnCondition(u_condition* uc, u_mutex* um);
void signalAll(u_condition* uc);
void signalOne(u_condition* uc);
void waitForThread(u_thread* ut);
