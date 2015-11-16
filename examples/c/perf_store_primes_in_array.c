/** @example perf_store_primes_in_array.c
 *  Store primes in a C array. 
 *   compare computing N arrays of size SZ vs one array of size (N*SZ)
*/

#include <primesieve.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

void one_cycle_array(uint64_t n, uint64_t start, uint64_t *array, uint64_t len)
{
  struct timeval tval_before, tval_after, tval_result;

  if ( len < n) return;
  
  printf("generating n primes for n=%llu starting at %llu\n", n, start);
  /* store the primes below 1000 */
  gettimeofday(&tval_before, NULL);

  (void)primesieve_generate_n_primes_array(n, start, array, len, UINT64_PRIMES);
  
  gettimeofday(&tval_after, NULL);
  timersub(&tval_after, &tval_before, &tval_result);
  printf("Time elapsed: %ld.%06ld sec\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
  
}

void one_cycle(uint64_t n, uint64_t start)
{
  struct timeval tval_before, tval_after, tval_result;
  
  printf("generating n primes for n=%llu starting at %llu\n", n, start);
  /* store the primes below 1000 */
  gettimeofday(&tval_before, NULL);
  
  uint64_t *primes = (uint64_t *) primesieve_generate_n_primes(n, start, UINT64_PRIMES);
  
  gettimeofday(&tval_after, NULL);
  timersub(&tval_after, &tval_before, &tval_result);
  printf("Time elapsed: %ld.%06ld sec\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
  
  primesieve_free(primes);

}

int main(int argc, char ** argv)
{
  uint64_t start = 0, cur_start;
  uint64_t nn = 1000000000;
  int      repeats = 10;
  int      ii;

  if (argc == 4) {
    nn      = strtoull(argv[1], NULL, 10);
    start   = strtoull(argv[2], NULL, 10);
    repeats = atoi(argv[3]);
  } else {
    char *s = strrchr(argv[0],'/');
    printf("usage: %s N-primes START REPEATS\n", s ? s+1: argv[0]);
    return 1;
  }

  printf("using existing interface for (n,start,repeats) = (%llu, %llu, %d)\n", nn, start, repeats);

  cur_start = start;
  for (ii = 0; ii < repeats; ii++) {
    one_cycle(nn, cur_start);
    cur_start += nn;
  }
  
  one_cycle(nn * repeats, 0);

  printf("using array interface for (n,start,repeats) = (%llu, %llu, %d)\n", nn, start, repeats);

  uint64_t *array;
  array = malloc( nn * sizeof(uint64_t));

  if (array == NULL) return 1;

  cur_start = start;
  for (ii = 0; ii < repeats; ii++) {
    one_cycle_array(nn, cur_start, array, nn);
    cur_start += nn;
  }
  free (array); array = NULL;

  array = malloc( nn * repeats * sizeof(uint64_t));
  if (array == NULL) return 1;

  one_cycle_array(nn * repeats, 0, array, nn*repeats);

  return 0;
}
