#include "general/logic.h" 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


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

  pthread_mutex_init(&pool->mutex, NULL);
}

void putRangeInPool(primes_range src_range, primes_pool* pool){
  pthread_mutex_lock(&(pool->mutex));
  primes_range* new_range=(primes_range*)malloc(sizeof(primes_range));
  *new_range=src_range; // copy src

  // client must himself clean numbers!
  //memset(new_range->numbers, 0, MAX_RANGE_SIZE);

  primes_range* cur_range=pool->first_range;
  primes_range* following_link=NULL;

  //while (!(cur_range->next_range == NULL)){
  while (!(cur_range == NULL)){
    //    if (new_range->lower_bound > cur_range->upper_bound){
    int check=checkRange(new_range, cur_range);
    printf("check\n");
    if (check > 0){
      // ok
      //new_range->next_range=cur_range->next_range;
      following_link=cur_range->next_range;
      //cur_range->next_range=new_range;
      break;
    } else if (check < 0){
      // error - no matching place
      printf("Range cannot be placed\n");
      pthread_mutex_unlock(&(pool->mutex));  
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
  printRangeStatus(new_range);
  
  pthread_mutex_unlock(&(pool->mutex));  
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

int checkRange(primes_range* to_put, primes_range* prev){
  unsigned int x=to_put->lower_bound, y=to_put->upper_bound;
  unsigned int ll=prev->lower_bound, lu=prev->upper_bound;
  unsigned int rl, ru;
  if (prev->next_range == NULL){
    ru=~0; // max unsigned int
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

  


void destroyPool(primes_pool* pool){
  pthread_mutex_lock(&(pool->mutex));
  primes_range* range=pool->first_range;
  primes_range* next;
  while (range != NULL){
    next=range->next_range;
    free(range);
    range=next;    
  }
  pthread_mutex_unlock(&(pool->mutex));

  pthread_mutex_destroy(&(pool->mutex));
}

void printPoolStatus(primes_pool* pool){
  pthread_mutex_lock(&(pool->mutex));
  primes_range* range=pool->first_range;
  while (range != NULL){
    printRangeStatus(range);
    range=range->next_range;
  }
  pthread_mutex_unlock(&(pool->mutex));
}

void printRangeStatus(primes_range* range){
  printf("Range %d .. %d\n", range->lower_bound, range->upper_bound);
  int* numbers=range->numbers;
  int i;
  
  for (i = 0; i<MAX_RANGE_SIZE; i++) {
    if (numbers[i] == 0) {
      break;
    }
    printf("%d, ", numbers[i]);
  }
  printf("\n");
}


/* void computeInRange(primes_range* range); */
/* int getCurrentMaxPrime(primes_pool* pool); */

