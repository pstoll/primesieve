/** @example store_primes_in_array.c
 *  Store primes in a C array. */

#include <primesieve.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

void one_cycle(uint64_t n, uint64_t start)
{
  struct timeval tval_before, tval_after, tval_result;
  
  printf("generating n primes for n=%llu starting at %llu\n", n, start);
  /* store the primes below 1000 */
  gettimeofday(&tval_before, NULL);
  
  int* primes = (int*) primesieve_generate_n_primes(n, start, INT_PRIMES);
  
  gettimeofday(&tval_after, NULL);
  timersub(&tval_after, &tval_before, &tval_result);
  printf("Time elapsed: %ld.%06ld\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
  
  primesieve_free(primes);

}

int main(int argc, char ** argv)
{
  uint64_t start = 0;
  uint64_t nn = 1000000000;
  int      repeats = 10;
  int      ii;

  if (argc == 4) {
    nn      = strtoull(argv[1], NULL, 10);
    start   = strtoull(argv[2], NULL, 10);
    repeats = atoi(argv[3]);
  }

  printf("comparing (n,start,repeats) = (%llu, %llu, %d)\n", nn, start, repeats);

  for (ii = 0; ii < repeats; ii++) {
    one_cycle(nn, start);
    start += nn;
  }
  
  one_cycle(nn * repeats, 0);
  
  return 0;
}
