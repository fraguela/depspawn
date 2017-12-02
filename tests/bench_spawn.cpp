/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2017 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

///
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <chrono>
#include <tbb/spin_mutex.h>  // This is only for serializing parallel prints
#include "depspawn/depspawn.h"

#define N      400000

using namespace depspawn;

typedef void (*voidfptr)();

tbb::spin_mutex  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)


std::chrono::time_point<std::chrono::high_resolution_clock> t0, t1, t2;

double total_time = 0.;

int mx[N][128/sizeof(int)];
int nsize = N;
int nthreads = tbb::task_scheduler_init::automatic;
int queue_limit = -1;
int retstate = 0;
int global_ntest; ///Used by pr()

void cleanmx() {
  memset((void *)mx, 0, sizeof(mx));
}

void pr(const char * s)
{
  double tot = std::chrono::duration<double>(t2 - t0).count();
  printf("T%2d %31s. ", global_ntest + 1, s);
  printf("Spawning : %8lf Running: %8lf T:%8lf\n", std::chrono::duration<double>(t1 - t0).count(), std::chrono::duration<double>(t2 - t1).count(), tot);
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
  
  t0 = std::chrono::high_resolution_clock::now();
  
  for(int i = 0; i < nsize; i++)
    spawn(f, std::move(i)); //This freezes the value
  
  t1 = std::chrono::high_resolution_clock::now();
  
  wait_for_all();
  
  t2 = std::chrono::high_resolution_clock::now();
  
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
  
  t0 = std::chrono::high_resolution_clock::now();
  
  for(int i = 0; i < nsize; i++)
    spawn(g, mx[i][0], mx[i][7], mx[i][1], mx[i][6], mx[i][2], mx[i][5], mx[i][3], mx[i][4]);
  
  t1 = std::chrono::high_resolution_clock::now();
  
  wait_for_all();
  
  t2 = std::chrono::high_resolution_clock::now();
  
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

  t0 = std::chrono::high_resolution_clock::now();
  
  for(int i = 0; i < nsize; i++)
    spawn(h, &mx[i][0], &mx[i][7], &mx[i][1], &mx[i][6], &mx[i][2], &mx[i][5], &mx[i][3], &mx[i][4]);
  
  t1 = std::chrono::high_resolution_clock::now();
  
  wait_for_all();
  
  t2 = std::chrono::high_resolution_clock::now();
  
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
  
  t0 = std::chrono::high_resolution_clock::now();
  
  spawn(rec, nsize-1);
  
  t1 = std::chrono::high_resolution_clock::now();
  
  wait_for_all();
  
  t2 = std::chrono::high_resolution_clock::now();
  
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
  
  t0 = std::chrono::high_resolution_clock::now();
  
  spawn(rec2, 0, nsize);
  
  t1 = std::chrono::high_resolution_clock::now();
  
  wait_for_all();
  
  t2 = std::chrono::high_resolution_clock::now();
  
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
  
  t0 = std::chrono::high_resolution_clock::now();
  
  spawn(depprev, mx[0][0], mx[0][0]);
  
  for(int i = 1; i < nsize; i++) {
    spawn(depprev, mx[i-1][0], mx[i][0]);
  }
  
  t1 = std::chrono::high_resolution_clock::now();
  
  wait_for_all();
  
  t2 = std::chrono::high_resolution_clock::now();
  
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
  
  t0 = std::chrono::high_resolution_clock::now();
  
  for(int i = 0; i < nsize; i++) {
    spawn(depsame, mx[0][0]);
  }
  
  t1 = std::chrono::high_resolution_clock::now();
  
  wait_for_all();
  
  t2 = std::chrono::high_resolution_clock::now();
  
  
  if(mx[0][0] != nsize) {
    std::cerr << " -> " << mx[0][0];
    doerror();
  }
  mx[0][0] = 0;

  
  pr("f(int&) depsame");
  
}

/** These three tests compare the performance when an input does not need to be tracked */
void test8()
{ int v;
  
  //V1: just use it "as is"
  v = 0;
  
  t0 = std::chrono::high_resolution_clock::now();
  
  for(int i = 0; i < nsize; i++) {
    spawn(depprev, nthreads, v);
  }
  
  t1 = std::chrono::high_resolution_clock::now();
  
  wait_for_all();
  
  t2 = std::chrono::high_resolution_clock::now();
  
  if(v != (nthreads + 1)) {
    std::cerr << " -> " << v;
    doerror();
  }
  
  pr("f(const_var, same_int&)");
  
  //V2: Freeze it
  v = 0;
  
  t0 = std::chrono::high_resolution_clock::now();
  
  for(int i = 0; i < nsize; i++) {
    spawn(depprev, std::move(nthreads), v);
  }
  
  t1 = std::chrono::high_resolution_clock::now();
  
  wait_for_all();
  
  t2 = std::chrono::high_resolution_clock::now();
  
  if(v != (nthreads + 1)) {
    std::cerr << " -> " << v;
    doerror();
  }
  
  pr("f(frozen const_var, same_int&)");
  
  //V3: Ignore it
  v = 0;
  
  t0 = std::chrono::high_resolution_clock::now();
  
  for(int i = 0; i < nsize; i++) {
    spawn(depprev, ignore(nthreads), v);
  }
  
  t1 = std::chrono::high_resolution_clock::now();
  
  wait_for_all();
  
  t2 = std::chrono::high_resolution_clock::now();
  
  if(v != (nthreads + 1)) {
    std::cerr << " -> " << v;
    doerror();
  }

  pr("f(ignore(const_var), same_int&)");
}

/** These three tests compare the performance when an input does not need to be tracked */
void test9()
{
  
  //V1: just use it "as is"
  t0 = std::chrono::high_resolution_clock::now();
  
  for(int i = 0; i < nsize; i++) {
    spawn(depprev, nthreads, mx[i][0]);
  }
  
  t1 = std::chrono::high_resolution_clock::now();
  
  wait_for_all();
  
  t2 = std::chrono::high_resolution_clock::now();
  
  for(int i = 0; i < nsize; i++) {
    if(mx[i][0] != (nthreads + 1)) {
      std::cerr << i << " -> " << mx[i][0];
      doerror();
    }
    mx[i][0] = 0;
  }
  
  pr("f(const_var, diff_int&)");
  
  //V2: Freeze it
  t0 = std::chrono::high_resolution_clock::now();
  
  for(int i = 0; i < nsize; i++) {
    spawn(depprev, std::move(nthreads), mx[i][0]);
  }
  
  t1 = std::chrono::high_resolution_clock::now();
  
  wait_for_all();
  
  t2 = std::chrono::high_resolution_clock::now();
  
  for(int i = 0; i < nsize; i++) {
    if(mx[i][0] != (nthreads + 1)) {
      std::cerr << i << " -> " << mx[i][0];
      doerror();
    }
    mx[i][0] = 0;
  }
  
  pr("f(frozen const_var, diff_int&)");
  
  //V3: Ignore it
  t0 = std::chrono::high_resolution_clock::now();
  
  for(int i = 0; i < nsize; i++) {
    spawn(depprev, ignore(nthreads), mx[i][0]);
  }
  
  t1 = std::chrono::high_resolution_clock::now();
  
  wait_for_all();
  
  t2 = std::chrono::high_resolution_clock::now();
  
  for(int i = 0; i < nsize; i++) {
    if(mx[i][0] != (nthreads + 1)) {
      std::cerr << i << " -> " << mx[i][0];
      doerror();
    }
    mx[i][0] = 0;
  }
  
  pr("f(ignore(const_var), diff_int&)");
}

//////////////////////////////////////////////////////
/////               Common part                  /////
//////////////////////////////////////////////////////

constexpr voidfptr tests[] =
 {test1, test2, test3,
  test4, test5, test6,
  test7, test8, test9 };

constexpr int NTESTS = sizeof(tests) / sizeof(tests[0]);
bool dotest[NTESTS];

void show_help()
{
  puts("bench_spawn [-h] [-q limit] [-t nthreads] [-T ntest] [problemsize]");
  puts("-h          Display help and exit");
  puts("-q limit    # of pending ready tasks that makes a spawning thread steal one");
  puts("-t nthreads Run with nthreads threads. Default is automatic (-1)");
  printf("-T ntest    Run test ntest in [1, %u]\n", NTESTS);
  puts("            Can be used multiple times to select several tests");
  puts("            By default all the tests are run\n");
  printf("problemsize defaults to %d\n", N);
}

int process_arguments(int argc, char **argv)
{ int c;
  bool tests_specified = false;

  for(c = 0; c < NTESTS; c++)
    dotest[c] = false;
  
  while ( -1 != (c = getopt(argc, argv, "hq:t:T:")) )
  {
    switch (c)
    {
      case 'q' : /* queue limit */
        queue_limit = atoi(optarg);
        break;
        
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
  
  set_threads(nthreads);

  if (queue_limit >= 0) {
    printf("Setting queue limit to %d\n", queue_limit);
    set_task_queue_limit(queue_limit);
  }
  
  cleanmx();
  
  for(global_ntest = 0; global_ntest < NTESTS; global_ntest++) {
    if(dotest[global_ntest])
      (*tests[global_ntest])();
  }
  
  printf("Total : %8lf\n", total_time);
  
  return retstate;
}
