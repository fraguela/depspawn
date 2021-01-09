/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2021 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
*/

/// g++ -O3 -DNDEBUG -o /tmp/a.out -std=c++0x -I${HOME}/Projects/depspawn/include fib.cpp /Users/basilio/Projects/depspawn/src/depspawn.cpp  -ltbb

///
/// \file     fib.cpp
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/time.h>
#include <depspawn/depspawn.h>

using namespace depspawn;

int p = 25;
int prune_limit = 20;
int nthreads = 4;
bool sequential = false;
bool pruned = false;
bool pure = false;

//std::thread::id tid;

void add(size_t& r, size_t i, size_t j) { r = i + j; }

//////////////

size_t sfib(int n) {
  
  if (n < 2) {
    return n;
  } else {
    return sfib(n - 1) + sfib(n - 2);
  }
  
}

void fib_debug(int n, int path, size_t& r) {
  if (n < 2) {
    r = n;
  } else {
    //printf("B%d %x %p (%c) w=%p\n", n, path, &r, std::this_thread::get_id() == tid ? 'M' : 'o', internal::enum_thr_spec_father.local());
    size_t i, j;
    spawn(fib_debug, n - 1, path<<1, i);
    fib_debug(n - 2, (path<<1)|1, j);
    wait_for_subtasks();
    //wait_for(i);
    //spawn(add, r, i, j);
    r = i + j;
    //printf("E%d %x->%lu %p(%lu+%lu) (%c)\n", n, path, r, &r, i, j, std::this_thread::get_id() == tid ? 'M' : 'o');
  }
}

void fib(int n, size_t& r) {
  if (n < 2) {
    r = n;
  } else {
    size_t i, j;
    spawn(fib, n - 1, i);
    fib(n - 2, j);
    wait_for_subtasks();
    //wait_for(i);
    //spawn(add, r, i, j);
    r = i + j;
  }
}

void fib_pruned(int n, size_t& r) {
  if (n < prune_limit) {
    r = sfib(n);
  } else {
    size_t i, j;
    spawn(fib_pruned, n - 1, i);
    fib_pruned(n - 2, j);
    wait_for_subtasks();
    //wait_for(i);
    //spawn(add, r, i, j);
    r = i + j;
  }
}

void usage(char *argv[])
{
  printf("%s [-s]equential [-p]runed [-P]ure [-T]hreads n [num] [limit]\n", argv[0]);
  exit(EXIT_FAILURE);
}

void config(int argc, char *argv[])
{ int i;
  
  while ((i = getopt(argc, argv, "spPT:")) != -1)
    switch(i) {
      case 's': sequential = true;
        break;
      case 'p': pruned = true;
        break;
      case 'P': pure = true;
        break;
      case 'T': nthreads = atoi(optarg);
        break;
      case '?':
      default:
        usage(argv);
    }
  
  if(optind >= argc) {
    usage(argv);
  }
  
  // optind must be < argc
  p = atoi(argv[optind++]);
  
  if(optind < argc) {
    prune_limit = atoi(argv[optind]);
  }
}

int main(int argc, char** argv) {
  size_t n;
  struct timeval t0, t1, t;
  
  config(argc, argv);

  printf("Using %d threads\n", nthreads);
  
  set_threads(nthreads);

  //tid = std::this_thread::get_id();
  //std::cout << "Main Thr=" <<  tid << std::endl;
  
  if (sequential) {
    gettimeofday(&t0, NULL);
    n = sfib(p);
    gettimeofday(&t1, NULL);
    timersub(&t1, &t0, &t);
    printf("seq fib(%d) = %lu\n", p, n);
    printf("compute time: %f\n", (t.tv_sec * 1000000 + t.tv_usec) / 1000000.0);
  }
  
  if (pruned) {
    gettimeofday(&t0, NULL);
    fib_pruned(p, n);
    wait_for_all();
    gettimeofday(&t1, NULL);
    timersub(&t1, &t0, &t);
    printf("pruned fib(%d) = %lu (Prune limit=%d)\n", p, n, prune_limit);
    printf("compute time: %f\n", (t.tv_sec * 1000000 + t.tv_usec) / 1000000.0);
  }
  
  if (pure) {
    gettimeofday(&t0, NULL);
    //fib_debug(p, 1, n);
    fib(p, n);
    wait_for_all();
    gettimeofday(&t1, NULL);
    timersub(&t1, &t0, &t);
    printf("pure fib(%d) = %lu\n", p, n);
    printf("compute time: %f\n", (t.tv_sec * 1000000 + t.tv_usec) / 1000000.0);
  }

  return 0;
}


/* 2 Thr
                       (5 1)
          (4 2)                        (3 3)
   (3 4)        (2 5)          (2 6)           (1 7)
 (2 8) (1 9)   1     0        1     0
*/
/*  1 Thr
                    (4 1)
         (3 2)  W-2-4       [(2 3)] W-1-6
    (2 4) E?  [(1 5)]   (1  6)   [(0  7)]
 (1 8) [(0 9)]
*/
