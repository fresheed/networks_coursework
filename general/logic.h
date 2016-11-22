//#include <pthread.h>
#include "general/u_threads.h"

#ifndef logic_structs
#define logic_structs

#define RANGE_UNKNOWN 0
/* #define RANGE_WAITING */
#define RANGE_COMPUTED 2

#define MAX_NUM 10000000
#define MAX_RANGE_SIZE 100000
#define MAX_NUM_LENGTH 8

typedef struct primes_range {
  unsigned long lower_bound, upper_bound;
  unsigned long numbers[MAX_RANGE_SIZE];
  char current_status;
  struct primes_range* next_range;
} primes_range;

typedef struct primes_pool {
  primes_range* first_range;
  unsigned long recent[MAX_RANGE_SIZE];
  u_mutex mutex;
} primes_pool;

#endif

void initPool(primes_pool* pool);
void destroyPool(primes_pool* pool);
void putRangeInPool(primes_range range, primes_pool* pool);
long checkRange(primes_range* to_put, primes_range* prev);

void printPoolStatus(primes_pool* pool);
void printRangeStatus(primes_range* range);

void getRecentPrimes(long amount, primes_pool* pool, long* res);
void updateRecent(primes_pool* pool, primes_range* range);

void computePrimesInRange(primes_range* range);

long getCurrentMaxPrime(primes_pool* pool);
long getPrimesCountInRange(primes_range* range);
