#include <windows.h>
#include <process.h>

#ifndef u_win_threads
#define u_win_threads

void createThread();
void createMutex();
void createCondition();
void destroyMutex();
void destroyCondition();
void signalOne();
void signalAll();
void waitForThread();

typedef struct u_thread {
    HANDLE hThread;
} u_thread;

typedef struct u_mutex {
    CRITICAL_SECTION crit_section;
} u_mutex;

typedef struct u_condition {
    CONDITION_VARIABLE ConditionVar;
} u_condition;


#endif // u_win_threads
