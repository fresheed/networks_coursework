#ifndef logic_structs
#define logic_structs

#define MAX_PRIMES 100
typedef struct primes {
  unsigned int numbers[MAX_PRIMES];
  unsigned int cur_num;
} primes;

#endif

void initPrimes(primes* data);
void computePrimes(unsigned int min, unsigned int max, int* buf);
void mergePrimes(primes* data, int* incoming);
int getCurrentMaxPrime(primes* data);


