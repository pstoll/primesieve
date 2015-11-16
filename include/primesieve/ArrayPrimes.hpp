///
/// @file   AppendArrayPrimes.hpp
/// @brief  This file contains classes needed to store primes in
///         a raw numeric array. These classes derive from Callback
///         and call PrimeSieve's callbackPrimes() method, the
///         primes are then stored at the end of the array inside the
///         callback method.
///
/// Copyright (C) 2014 Kim Walisch, <kim.walisch@gmail.com>
/// Copyright (C) 2015 Perry Stoll, <perry@pstoll.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#ifndef ARRAYPRIMES_HPP
#define ARRAYPRIMES_HPP

#include "PrimeSieve.hpp"
#include "ParallelPrimeSieve.hpp"
#include "Callback.hpp"
#include "cancel_callback.hpp"

#include <stdint.h>
#include <cmath>
#include <cstdio>
#include <iostream>

namespace primesieve {

/// approximate_prime_count(x) > pi(x)
/*
inline uint64_t approximate_prime_count(uint64_t start, uint64_t stop)
{
  if (start > stop)
    return 0;
  if (stop < 10)
    return 10;
  return static_cast<uint64_t>((stop - start) / (std::log(static_cast<double>(stop)) - 1.1));
}
*/

template <typename T>
class ArrayPrimes : public Callback<uint64_t>
{
public:
  ArrayPrimes(T* primes, uint64_t len)
    : array_(primes), arr_len_(len), idx_(0), extra_(0)
  { }
  uint64_t arrayPrimes(uint64_t start, uint64_t stop)
  {
    if (start <= stop)
    {
      PrimeSieve ps;
      ps.callbackPrimes(start, stop, this);
    }
    return idx_;
  }
  void callback(uint64_t prime)
  {
    if (idx_ < arr_len_) {
      array_[idx_] = static_cast<T>(prime);
      idx_++;
    } else {
      extra_++;
    }
  }
private:
  ArrayPrimes(const ArrayPrimes&);
  void operator=(const ArrayPrimes&);
  T array_[];
  uint64_t arr_len_;
  uint64_t idx_;
  uint64_t extra_;		// count of primes generated that did not fit into array
};


template <typename T>
class Array_N_Primes : public Callback<uint64_t>
{
public:
  Array_N_Primes(T* primes, uint64_t len) 
    : array_(primes), arr_len_(len)
  { }
  void array_N_Primes(uint64_t n, uint64_t start)
  {
    n_ = n;
    idx_ = 0;
    if (arr_len_ < n_) {
      // error
    }
    PrimeSieve ps;
    try
    {
      while (n_ > 0)
      {
        uint64_t logn = 50;
        // choose stop > nth prime
        uint64_t stop = start + n_ * logn + 10000;
        ps.callbackPrimes(start, stop, this);
        start = stop + 1;
      }
    }
    catch (cancel_callback& c) { }
  }
  void callback(uint64_t prime)
  {
    array_[idx_++] = static_cast<T>(prime);
    if (--n_ == 0)
      throw cancel_callback();
  }
  private:
    Array_N_Primes(const Array_N_Primes&);
    void operator=(const Array_N_Primes&);
    T* array_;
    uint64_t n_;
    uint64_t arr_len_;
    uint64_t idx_;
};

template <typename T>
class Array_N_Parallel_Primes : public Callback<uint64_t, int>
{
public:
  Array_N_Parallel_Primes(T* primes, uint64_t len) 
    : array_(primes), arr_len_(len), indexes_(0), abort_(false)
  {
    // we probably need scratch space in primes, e.g. 10% larger.
    // each thread will encounter primes indepdenently, can't guarantee that they will only generate 1/nth of the values
    //
    // so need some extra space for imbalance in prime number generation
    // so... do we really use the input array for computation?
    //       or do we allocate our own arrays and and copy into the output? is there a burden on the caller to supply enough extra space?
    //
  }

  virtual bool abort() { return abort_;}
  
  void array_N_Parallel_Primes(uint64_t n, uint64_t start)
  {
    n_ = n;

    // do we have enough output space to store primes?
    // in fact, we need probably arr_len_ > 1.1*n or so
    if (n > arr_len_) {
      return;
    }

    ParallelPrimeSieve pps;
    pps.setStart(start);
    pps.setStop(start + n);
    int nthreads = pps.getNumThreads();
    if (nthreads < 0) {
      nthreads = 1;
    }
    pps.setNumThreads(nthreads);
    printf("\tindexes_=%p\n", indexes_); fflush(stdout);
    init_thread_info(nthreads);
    printf("\tnthreads=%d\n", nthreads); fflush(stdout);
    try
    {
      // consider: compute 95% of values in parallel then compute last 5% single threaded
      //   this way we don't have to worry about the parallel merge of uncertain order of multiple threads
      
      uint64_t cur_n = current_count();
      uint64_t target_n = n_;
      bool do_single_thread_to_finish = false;
      if (do_single_thread_to_finish) {
	target_n = (uint64_t)(n_ * .99);
      }
      while (cur_n < target_n){
	printf("\tcur_n=%llu, target_n=%llu, n_ = %llu\n", cur_n, target_n, n_); fflush(stdout);
        uint64_t logn = 50;
        // choose stop > nth prime
        uint64_t stop = start + (target_n - cur_n) * logn + 10000;
	pps.setStart(start);
	pps.setStop(stop);
	printf("\tstart=%llu, stop=%llu\n", start, stop); fflush(stdout);
        pps.callbackPrimes(start, stop, this);
        start = stop + 1;
	cur_n = current_count();
	printf("\tcur_n=%llu, new_start=%llu\n", cur_n, start); fflush(stdout);
	// assert(cur_n == start)?
      }
      if (do_single_thread_to_finish) {
	printf("Going single threaded to finish up\n"); fflush(stdout); 
	squash_data();
	init_thread_info(1);
	start = array_[cur_n-1];	// errr....probably need to sort this thing? argh.
	pps.setNumThreads(1);
	indexes_[0] = cur_n;
	printf("\tcur_n=%llu, start=%llu, stop=%llu\n", cur_n, start, n_); fflush(stdout);
	pps.setStart(start);
	pps.setStop(n_);
	pps.callbackPrimes(start, n_, this);
      }
      
    }
    catch(cancel_callback& e) {
      std::cout << "cancel_callback caught" << std::endl;
      std::cout << e.what() << std::endl;
    }
    catch(std::exception& e) {
      std::cout << "std::exception caught" << std::endl;
      std::cout << e.what() << std::endl;
    }
    catch (...) {
      std::cout << "caught everything" << std::endl;
    }

    squash_data();
    if (indexes_) {
      delete indexes_;
      indexes_ = NULL;
    }

  }

  inline const uint64_t  get_thread_offset(int threadnum) {
    return threadnum * len_per_thread_;
  }
  
  void callback(uint64_t prime, int thread_num)
  {
    const uint64_t thread_index = indexes_[thread_num];    // no thread should do more than slots_per_thread work
    const uint64_t arr_index    = thread_index + get_thread_offset(thread_num); // globally, do not go beyond arr_len_ items

    bool can_continue = true;
    if (thread_index >= len_per_thread_) {
      // exceeded number of values any one thread should generate
      printf("TID=%d thread offset exceeds per thread storage len\n", thread_num); fflush(stdout);
      can_continue = false;
    }
    if (arr_index >= arr_len_) {
      printf("TID=%d arr_index exceeds total storage len\n",thread_num); fflush(stdout);
      // we have filled up the entire output buffer, also bad
      can_continue = false;
    }

    if (!can_continue) {
      printf("TID=%d thread_index=%llu, len_per_thread_=%llu\n",
	     thread_num, thread_index, len_per_thread_ ); fflush(stdout); 
      printf("TID=%d arr_index=%llu arr_len_=%llu\n",
	     thread_num, arr_index, arr_len_ ); fflush(stdout); 
      abort_ = true;
      //throw cancel_callback();
    }
    
    array_[arr_index] = static_cast<T>(prime);
    indexes_[thread_num]++;
    
  }
  private:

    void init_thread_info( int nthreads) {
      nthreads_ = nthreads;
      if (indexes_) { delete indexes_; indexes_ = NULL; }
      indexes_  = new uint64_t[nthreads_];
      std::fill(indexes_, indexes_ + nthreads_, 0);
      primes_per_thread_  =       n_ / nthreads_;
      len_per_thread_     = arr_len_ / nthreads_;
    }
  
    //  current_count: sum number of values computed so far across all the work threads
    const uint64_t current_count() {
      uint64_t n = 0;
      if (indexes_ != NULL) {
	for(int ii = 0; ii < nthreads_; ii++) {
	  n += indexes_[ii];
	  printf("current_count: indexes_[%d] = %llu\n", ii, indexes_[ii]);
	  fflush(stdout);
	}
      }
      printf("current_count: total = %llu\n",n);
      fflush(stdout);
      return n;
    }

  // we needed to leave blank unfilled entries at the end of each range
  // this function consolidates the data to densely fill the array starting at 0
  // returns: number of values total in the output array.
  //    turns out this should be the same as 
  uint64_t squash_data() {
    uint64_t curr;		//  offset into current value
    if (indexes_  == NULL) {
      return 0;
    }
    uint64_t tid = 0;		// thread id

    // optimization: fast forward thread 0
    curr = indexes_[0];

    // now shift down all the other values in each block down to be contiguous with the values in block 0
    for(tid = 1; tid < nthreads_; tid++) {
      const uint64_t thread_offset = get_thread_offset(tid);
      const uint64_t thread_vals = indexes_[tid];

      for(uint64_t jj = 0; jj < thread_vals; jj++) {
	array_[curr++] = array_[jj + thread_offset];
      }
    }
    return curr;
  }
  
    Array_N_Parallel_Primes(const Array_N_Parallel_Primes&);
    void operator=(const Array_N_Parallel_Primes&);

    // primes array - each thread gets a range to fill.
    //  e.g. thread i gets range [i*slots_per_thread, (i+1)*slots_per_thread)
    T* array_;
    uint64_t arr_len_;		  // len of array_
    uint64_t len_per_thread_;	  // length of output array per thread
    uint64_t n_;		  // number of primes to generate total
    uint64_t primes_per_thread_;  // number of primes to generate per thread

    uint64_t nthreads_;		  // number of threads we are going to use

    uint64_t *indexes_;     	  // per thread count of values stored so far
    bool abort_;
};

} // namespace primesieve

#endif
