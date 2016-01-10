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

#include <iostream>
//#include <algorithm>
#include <tbb/task_scheduler_init.h>
#include <tbb/tick_count.h>
#include <tbb/atomic.h>
#include <tbb/spin_mutex.h>  // This is only for serializing parallel prints

#include "depspawn/depspawn.h"

using namespace boost;
using namespace depspawn;

tbb::spin_mutex  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)

typedef int ct22[22];

volatile bool inwritemode = false;

tbb::tick_count t0;

bool second_round = false;

tbb::atomic<int> x, success_counter;

void fun(bool readermode, int n = 600) {
  volatile double a = 2.1, b = 3.2, c=-42.1;
  
  if(readermode && inwritemode) LOG("ERROR!");
  else success_counter++;
  
  for(int i = 0; i < n; i++)
    for(int j = 0; j < n; j++)
      for(int k = 0; k < n; k++)
        a = a*b/c;
  if(readermode && inwritemode) LOG("ERROR!");
  else success_counter++;
}

// Because the input is a pointer, the region pointed is not tracked and, in theory it can be read or written
//in parallel with other reads or writes. In practice, it can be read or written in parallel with other reads
//but not with writes because the whole input array is used in the invocation and it is tracked as a "read"
//because the pointer is just read, as it is passed by value. Same in f5.
void f1(int a[10])
{ 
  LOG("f1 " << a[0] << " begin: " << (tbb::tick_count::now() - t0).seconds()); 
  a[0]++;
  fun(true);
  LOG("f1 " << a[0] << " end: " << (tbb::tick_count::now() - t0).seconds());
  x.fetch_and_increment();
}

void f2(const int (&a)[22])
{ 
  success_counter.fetch_and_add((second_round && (x<0)) || (!second_round && (x>=0)));
  
  LOG("f2 " << a[0] << " begin: " << (tbb::tick_count::now() - t0).seconds()); 
  //a[0]++;
  fun(true);
  LOG("f2 " << a[0] << " end: " << (tbb::tick_count::now() - t0).seconds());
  x.fetch_and_add(8);
  success_counter.fetch_and_add((second_round && (x<0)) || (!second_round && (x>=0)));
}

void f2b(const int (&a)[22])
{ 
  success_counter.fetch_and_add((second_round && (x<0)) || (!second_round && (x>=0)));
  LOG("f2b " << a[0] << " begin: " << (tbb::tick_count::now() - t0).seconds());
  //a[0]++;
  fun(true);
  LOG("f2b " << a[0] << " end: " << (tbb::tick_count::now() - t0).seconds());
  x.fetch_and_add(16);
  success_counter.fetch_and_add((second_round && (x<0)) || (!second_round && (x>=0)));
}

void f2c(const ct22& a)
{ 
  success_counter.fetch_and_add((second_round && (x<0)) || (!second_round && (x>=0)));
  LOG("f2c " << a[0] << " begin: " << (tbb::tick_count::now() - t0).seconds()); 
  //a[0]++;
  fun(true);
  LOG("f2c " << a[0] << " end: " << (tbb::tick_count::now() - t0).seconds());
  x.fetch_and_add(32);
  success_counter.fetch_and_add((second_round && (x<0)) || (!second_round && (x>=0)));
}

void f3(ct22& a)
{ 
  success_counter.fetch_and_add(x >= (32 + 16 + 8)); //f2, f2b and f2c must have finished
  inwritemode = true;
  LOG("-------------\nf3 " << a[0] << " begin: " << (tbb::tick_count::now() - t0).seconds());
  a[0]++;
  fun(false);
  LOG("f3 " << a[0] << " end: " << (tbb::tick_count::now() - t0).seconds());
  inwritemode = false;
  x.fetch_and_add(-(32 + 16 + 8));
}

void f3b(int (&a)[22])
{ 
  success_counter.fetch_and_add((x >= 0) && (x < 8)); //f3 must have finished
  inwritemode = true;
  LOG("f3b " << a[0] << " begin: " << (tbb::tick_count::now() - t0).seconds());
  a[0]++;
  fun(false);
  LOG("f3b " << a[0] << " end: " << (tbb::tick_count::now() - t0).seconds() << "\n-------------");
  inwritemode = false;
  x = -256;
  second_round = true;
}

void f5(int a[10]) //identical to f1. Just different name
{ 
  LOG("f5 " << a[0] << " begin: " << (tbb::tick_count::now() - t0).seconds()); 
  fun(true);
  LOG("f5 " << a[0] << " end: " << (tbb::tick_count::now() - t0).seconds());
  x.fetch_and_add(2);
}
  
int main(int argc, char **argv) 
{ int a[22], tmp;
  
  a[0] = 0;
  
  const int nthreads = (argc == 1) ? 8 : atoi(argv[1]);
  
  tbb::task_scheduler_init tbbinit(nthreads);
  
  std::cout << "Running with " << nthreads << " threads" << std::endl;
  
  x = 0;
  success_counter = 0;
  
  t0 = tbb::tick_count::now();
  
  spawn(f1, a);
  
  spawn(f2, a);
  
  spawn(f2b, a);
  
  spawn(f2c, a);
  
  // Write
  spawn(f3, a);
  
  //Write
  spawn(f3b, a);
  
  spawn(f5, a);
  
  spawn(f1, a);
  
  spawn(f2, a);
  
  spawn(f2b, a);
  
  spawn(f2c, a);
  
  wait_for_all();
  
  tmp = success_counter; //11 f * 2 + 6 * 2 + 2 = 36
  
  const bool test_ok = (tmp == 36);

  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}

