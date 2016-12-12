#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "general/logic.h"


void initPool(primes_pool* pool){
  pool->first_range=NULL;
  primes_range first;
  memset(first.numbers, 0, MAX_RANGE_SIZE);

  first.lower_bound=2;
  first.upper_bound=3;
  first.numbers[0]=2;
  first.numbers[1]=3;
  first.current_status=RANGE_COMPUTED;

  primes_range* new_range=(primes_range*)malloc(sizeof(primes_range));
  *new_range=first;
  new_range->next_range=NULL;
  pool->first_range=new_range;

  memset(pool->recent, 0, MAX_RANGE_SIZE);

  createMutex(&(pool->mutex));
}

void smth(){

}

void putRangeInPool(primes_range* src_range, primes_pool* pool){
  lockMutex(&(pool->mutex));

  primes_range* new_range=(primes_range*)malloc(sizeof(primes_range));
  *new_range=*src_range; // copy src

  // client must himself clean numbers!
  //memset(new_range->numbers, 0, MAX_RANGE_SIZE);

  primes_range* cur_range=pool->first_range;
  primes_range* following_link=NULL;

  //while (!(cur_range->next_range == NULL)){
  while (!(cur_range == NULL)){
    //    if (new_range->lower_bound > cur_range->upper_bound){
    int check=checkRange(new_range, cur_range);
    if (check > 0){
      // ok
      //new_range->next_range=cur_range->next_range;
      following_link=cur_range->next_range;
      //cur_range->next_range=new_range;
      break;
    } else if (check < 0){
      // error - no matching place
      printf("Range cannot be placed\n");
      unlockMutex(&(pool->mutex));
      return;
    } else {
      // continue check
      cur_range=cur_range->next_range;
    }
  }

  // otherwise place it in the end
  new_range->next_range=following_link;
  cur_range->next_range=new_range;

  printf("Added range:\n");
  printRangeStatus(new_range, 0);

  updateRecent(pool, new_range);
  unlockMutex(&(pool->mutex));
}


int checkRange2(primes_range* to_put, primes_range* prev){
  if (to_put->lower_bound > prev->upper_bound){
    if ((prev->next_range == NULL) || (to_put->upper_bound < prev->next_range->lower_bound)){
      return 1;
    } else {
      perror("-\n");
      return 0;
    }
  } else if (to_put->lower_bound <= prev->upper_bound) {
    perror("--\n");
    return -1;
  }
  return 0;
}

long checkRange(primes_range* to_put, primes_range* prev){
  unsigned long x=to_put->lower_bound, y=to_put->upper_bound;
  //unsigned long ll=prev->lower_bound, lu=prev->upper_bound;
  unsigned long lu=prev->upper_bound;
  unsigned long rl, ru;
  if (prev->next_range == NULL){
    ru=~0; // max unsigned int/long
    rl=ru-1;
  } else {
    rl=prev->next_range->lower_bound;
    ru=prev->next_range->upper_bound;
  }

  /* if ((x <= lu) || (rl <= y)){ */
  /*   return -1; */
  /* } */
  if (x <= lu){
    return -1;
  } else {
    if (rl <= y){
      if (ru <= x){
	// may be above next
	return 0;
      }
      return -1;
    } else {
      return 1;
    }
  }
  return 0;
}

int validateRangeParams(long lower, long upper){
  if (lower <= 0 || upper <= 0){
    printf("Negative values not allowed\n");
    return 0;
  }
  if (lower >= upper) {
    printf("Lower > upper\n");
    return 0;
  }
  if (upper-lower > MAX_RANGE_SIZE){
    printf("Range size = %ld > max size\n", (upper-lower));
    return 0;
  }
  return 1;
}



void destroyPool(primes_pool* pool){
  lockMutex(&(pool->mutex));
  primes_range* range=pool->first_range;
  primes_range* next;
  while (range != NULL){
    next=range->next_range;
    free(range);
    range=next;
  }
  unlockMutex(&(pool->mutex));

  destroyMutex(&(pool->mutex));
}

void printPoolStatus(primes_pool* pool, int print_numbers){
  lockMutex(&(pool->mutex));
  primes_range* range=pool->first_range;
  while (range != NULL){
    printRangeStatus(range, print_numbers);
    range=range->next_range;
  }
  unlockMutex(&(pool->mutex));
}

void printRangeStatus(primes_range* range, int print_numbers){
  printf("Range %ld .. %ld\n", range->lower_bound, range->upper_bound);
  if (!print_numbers){
    return;
  }
  unsigned long* numbers=range->numbers;
  long i;

  for (i = 0; i<MAX_RANGE_SIZE; i++) {
    if (numbers[i] == 0) {
      break;
    }
    printf("%ld, ", numbers[i]);
  }
  printf("\n");
}

void computePrimesInRange(primes_range* range){
  long pos=0;
  long num;
  long divisor;
  int is_prime;
  for (num = range->lower_bound; num<=range->upper_bound; num++) {
    if (num % 2 == 0) continue;
    is_prime=1;
    long max_divisor=(int)sqrt(num) + 1;
    for (divisor=3; divisor<=max_divisor; divisor+=2) {
      if (num % divisor == 0) {
	is_prime=0;
	break;
      }
    }
    if (is_prime){
      range->numbers[pos++]=num;
    }
  }
  range->current_status=RANGE_COMPUTED;
}

void getRecentPrimes(long amount, primes_pool* pool, unsigned long* res){
  lockMutex(&(pool->mutex));
  long i;
  for (i = 0; i<amount; i++) {
    res[i]=pool->recent[i];
  }
  unlockMutex(&(pool->mutex));
}

void updateRecent(primes_pool* pool, primes_range* range){
  long new_len=getPrimesCountInRange(range);
  long kept=MAX_RANGE_SIZE-new_len;
  long i;
  for (i = kept-1; i>=0; i--) {
    pool->recent[i+new_len]=pool->recent[i];
  }
  for (i=0; i<new_len; i++) {
    pool->recent[i]=range->numbers[i];
  }
}

void setRangeNumbers(primes_range* range, unsigned long* numbers, unsigned long len){
  long i;
  for (i = 0; i<len; i++) {
    range->numbers[i]=numbers[i];
  }
  range->current_status=RANGE_COMPUTED;
}

long getPrimesCountInRange(primes_range* range){
  long i;
  for (i = 0; i<MAX_RANGE_SIZE; i++) {
    if (range->numbers[i] == 0){
      return i;
    }
  }
  return MAX_RANGE_SIZE;
}

long getCurrentMaxPrime(primes_pool* pool){
  lockMutex(&(pool->mutex));
  primes_range* range=pool->first_range;
  while (range->next_range != NULL){
    range=range->next_range;
  }
  // now it points to last range
  long total_in_range=getPrimesCountInRange(range);
  long max=range->numbers[total_in_range-1];
  unlockMutex(&(pool->mutex));
  return max;
}

