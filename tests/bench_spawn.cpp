/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2016 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 This file is part of DepSpawn.
 
 DepSpawn is free software; you can redistribute it and/or modify it under the terms
 of the GNU General Public License as published by the Free Software Foundation;
 either version 2 as published by the Free Software Foundation.
 
 DepSpawn is distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with Threading Building Blocks; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 As a special exception, you may use this file as part of a free software
 library without restriction.  Specifically, if other files instantiate
 templates or use macros or inline functions from this file, or you compile
 this file and link it with other files to produce an executable, this
 file does not by itself cause the resulting executable to be covered by
 the GNU General Public License.  This exception does not however
 invalidate any other reasons why the executable file might be covered by
 the GNU General Public License.
 */

///
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <tbb/task_scheduler_init.h>
#include <tbb/tick_count.h>
#include <tbb/spin_mutex.h>  // This is only for serializing parallel prints
#include "depspawn/depspawn.h"

#define N      100000
#define NTESTS      7

using namespace depspawn;

typedef void (*voidfptr)();

tbb::spin_mutex  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)


tbb::tick_count t0, t1, t2;

double total_time = 0.;

int mx[N][128/sizeof(int)];
int nsize, nthreads, retstate = 0;
bool dotest[NTESTS];
voidfptr tests[NTESTS];

void cleanmx() {
  bzero(mx, sizeof(mx));
}

void pr(const char * s)
{
  double tot = (t2 - t0).seconds();
  printf("%27s %u times. ", s, nsize);
  printf("Spawning : %8lf Running: %8lf T:%8lf\n", (t1 - t0).seconds(), (t2 - t1).seconds(), tot);
  total_time += tot;
}

void doerror()
{
  retstate = -1;
  std::cerr << "Error\n";
}

/*
void debug(tbb::task& t, int nsp = 0) {
  internal::Workitem *p = ((internal::runner<internal::Launcher, void *> *) &t)->ctx_;
  LOG('S' << nsp << ' ' << p->ndependencies << ' ' << p->status);
}
*/

/****************************************************************/

void f(int i) {
  mx[i][0]++;
  // LOG(i << "->" << mx[i][0]);
}

/** A single main thread spawns nsize tasks with a single frozen input (copied).
    This also implies they are all independent
   */
void test1() 
{
  
  t0 = tbb::tick_count::now();
  
  for(int i = 0; i < nsize; i++)
    spawn(f, std::move(i)); //This freezes the value
  
  t1 = tbb::tick_count::now();
  
  wait_for_all();
  
  t2 = tbb::tick_count::now();
  
  for(int i = 0; i < nsize; i++) {
    if(mx[i][0] != 1) doerror();
    mx[i][0] = 0;
  }
  
  pr("f(frozen int)");
  
}

void g(int &i0, int &i1, int& i2, int& i3, int& i4, int& i5, int& i6, int& i7) {
  i0++;
  i1++;
  i2++;
  i3++;
  i4++;
  i5++;
  i6++;
  i7++;
}

/** A single main thread spawns nsize tasks with 8 arguments by reference.
    All the tasks are independent.
 */
void test2() 
{
  
  t0 = tbb::tick_count::now();
  
  for(int i = 0; i < nsize; i++)
    spawn(g, mx[i][0], mx[i][7], mx[i][1], mx[i][6], mx[i][2], mx[i][5], mx[i][3], mx[i][4]);
  
  t1 = tbb::tick_count::now();
  
  wait_for_all();
  
  t2 = tbb::tick_count::now();
  
  for(int i = 0; i < nsize; i++) {
    for (int j = 0; j < 8; ++j) {
      if(mx[i][j] != 1) doerror();
      mx[i][j] = 0;
    }
  }
  
  pr("f(8 int&) without overlaps");
  
}

void h(int *i0, int *i1, int* i2, int* i3, int* i4, int* i5, int* i6, int* i7) {
  (*i0)++;
  (*i1)++;
  (*i2)++;
  (*i3)++;
  (*i4)++;
  (*i5)++;
  (*i6)++;
  (*i7)++;
}

/** A single main thread spawns nsize tasks with 8 frozen pointers (they are rvalues).
    All the tasks are independent.
 */
void test3() 
{

  t0 = tbb::tick_count::now();
  
  for(int i = 0; i < nsize; i++)
    spawn(h, &mx[i][0], &mx[i][7], &mx[i][1], &mx[i][6], &mx[i][2], &mx[i][5], &mx[i][3], &mx[i][4]);
  
  t1 = tbb::tick_count::now();
  
  wait_for_all();
  
  t2 = tbb::tick_count::now();
  
  for(int i = 0; i < nsize; i++) {
    for (int j = 0; j < 8; ++j) {
      if(mx[i][j] != 1) doerror();
      mx[i][j] = 0;
    }
  }
  
  pr("f(8 frozen int*)");
  
}

void rec(int i) {
  if (i) {
    spawn(rec, i-1);
  };
  mx[i][0]++;
}

/** Recursive spawn in which each task spawns a single child task with a frozen input by value.
 */
void test4() 
{
  
  t0 = tbb::tick_count::now();
  
  spawn(rec, nsize-1);
  
  t1 = tbb::tick_count::now();
  
  wait_for_all();
  
  t2 = tbb::tick_count::now();
  
  for(int i = 0; i < nsize; i++) {
    if(mx[i][0] != 1) doerror();
    mx[i][0] = 0;
    for (int j=1; j < 8; ++j)
      if(mx[i][j] != 0) doerror();
  }
  
  pr("f(frozen int) rec1");
  
}

void rec2(int b, int e) {
  //LOG(b << ' ' << e);
  if (e <= (b+2)) {
    mx[b][0]++;
    if (e == (b+2))
      mx[b+1][0]++;
  } else {
    const int half = (b + e) / 2;
    spawn(rec2, std::move(b), std::move(half));
    spawn(rec2, std::move(half), std::move(e));
  }
  
}

/** Recursive spawn in which each task spawns two children to process the two halfs of its domain
   until chunks of just one or two elements are reached. All the arguments are frozen ints by value.
   All the tasks are thus independent.
 */
void test5() 
{
  
  t0 = tbb::tick_count::now();
  
  spawn(rec2, 0, nsize);
  
  t1 = tbb::tick_count::now();
  
  wait_for_all();
  
  t2 = tbb::tick_count::now();
  
  for(int i = 0; i < nsize; i++) {
    if(mx[i][0] != 1) doerror();
    mx[i][0] = 0;
    for (int j=1; j < 8; ++j)
      if(mx[i][j] != 0) doerror();
  }
  
  pr("f(frozen int) rec2");
  
}

void depprev(const int prev, int& out) {
  //LOG("B " << (&out - (int*)mx) / (128/sizeof(int)) );
  out = prev + 1;
  //LOG("E " << (&out - (int*)mx) / (128/sizeof(int)) );
}

/** A single main thread spawns nsize tasks in which task i depends (on an arg by value) 
    on task i-1 (on an arg it writes by reference).
 */
void test6() 
{
  
  t0 = tbb::tick_count::now();
  
  spawn(depprev, mx[0][0], mx[0][0]);
  
  for(int i = 1; i < nsize; i++) {
    spawn(depprev, mx[i-1][0], mx[i][0]);
  }
  
  t1 = tbb::tick_count::now();
  
  wait_for_all();
  
  t2 = tbb::tick_count::now();
  
  for(int i = 0; i < nsize; i++) {
    if(mx[i][0] != (i+1)) {
      std::cerr << i << " -> " << mx[i][0];
      doerror();
    }
    mx[i][0] = 0;
  }
  
  pr("f(int, int&) dep->");
  
}

void depsame(int& iout) {
  //LOG("B " << (&out - (int*)mx) / (128/sizeof(int)) );
  iout++;
  //LOG("E " << (&out - (int*)mx) / (128/sizeof(int)) );
}

/** A single main thread spawns nsize tasks all of which depend on the same element by reference */
void test7() 
{
  
  t0 = tbb::tick_count::now();
  
  for(int i = 0; i < nsize; i++) {
    spawn(depsame, mx[0][0]);
  }
  
  t1 = tbb::tick_count::now();
  
  wait_for_all();
  
  t2 = tbb::tick_count::now();
  
  
  if(mx[0][0] != nsize) {
    std::cerr << " -> " << mx[0][0];
    doerror();
  }
  mx[0][0] = 0;

  
  pr("f(int&) depsame");
  
}

void show_help()
{
  puts("bench_spawn [-h] [-t nthreads] [-T ntest] [problemsize]");
  puts("-h          Display help and exit");
  puts("-t nthreads Run with nthreads threads. Default is automatic (-1)");
  printf("-T ntest    Run test ntest in [1, %u]\n", NTESTS);
  puts("            Can be used multiple times to select several tests");
  puts("            By default all the tests are run\n");
  printf("problemsize defaults to %d\n", N);
}

int process_arguments(int argc, char **argv)
{ int c;
  bool tests_specified = false;
  
  nsize = N;
  nthreads = tbb::task_scheduler_init::automatic;
  for(c = 0; c < NTESTS; c++)
    dotest[c] = false;
  tests[0] = test1;
  tests[1] = test2;
  tests[2] = test3;
  tests[3] = test4;
  tests[4] = test5;
  tests[5] = test6;
  tests[6] = test7;
  
  while ( -1 != (c = getopt(argc, argv, "ht:T:")) )
  {
    switch (c)
    {
      case 't': /* threads */
	nthreads = atoi(optarg);
	break;
	
      case 'T': /* tests */
	c = atoi(optarg);
	if(c < 1 || c >  NTESTS) {
	  printf("The test number must be in [1, %u]\n", NTESTS);
	  return -1;
	}
	dotest[c - 1] = true;
	tests_specified = true;
	break;
	
      case 'h':
	show_help();
	
      default: /* unknown or missing argument */
	return -1;     
    } /* switch */
  } /* while */
  
  if (optind < argc) {
    nsize = atoi(argv[optind]); /* first non-option argument */
  }
  
  if(nsize > N) {
    printf("The problem size cannot exceed %i\n", N);
    return -1;
  }
  
  if(!tests_specified)
    for(c = 0; c < NTESTS; c++)
      dotest[c] = true;
  
  printf("Running problem size %u with %i threads (sizeof(int)=%zu)\n", nsize, nthreads, sizeof(int));
  
  return 0;
}

int main(int argc, char **argv)
{
  if(process_arguments(argc, argv) == -1)
    return -1;
  
#ifdef FAST_START
  set_threads(nthreads);
  set_task_queue_limit(100);
#else
  tbb::task_scheduler_init tbbinit(nthreads);
#endif
  
  cleanmx();
  
  for(int i = 0; i < NTESTS; i++)
    if(dotest[i])
      (*tests[i])();

  printf("Total : %8lf\n", total_time);
  
  return retstate;
}
