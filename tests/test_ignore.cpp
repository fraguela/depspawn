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

#include <iostream>
#include <ctime>
#include <tbb/compat/thread>
#include <tbb/tick_count.h>
#include <tbb/spin_mutex.h>  // This is only for serializing parallel prints
#include "depspawn/depspawn.h"

using namespace depspawn;

tbb::spin_mutex  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)


tbb::tick_count t0;

void mywait(float seconds)
{
  clock_t endwait;
  endwait = clock() + seconds * CLOCKS_PER_SEC ;
  while (clock() < endwait) {}
}

void f(volatile int &i, int j) {
  LOG("f begin: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i << " j=" << j);
  
  mywait(1.5f);
  
  LOG("f comes back at: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i << " j=" << j);
  
  while (i != 3) {
    //g should be able to run before this point of f, and thus let i=3
  }
  
  i += j;
  
  LOG("f finish: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i << " j=" << j);
}

void g(volatile int &i, int j) {
  LOG("g begin: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i << " j=" << j);
  
  while(!i) {
    //ignored_dep should be able to run before this point, letting i==1
  }
  i += j;
  
  LOG("g finish: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i << " j=" << j);
}

void h(volatile int &i, int j) {
  LOG("h begin: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i << " j=" << j);
  

  i += j;
  
  LOG("h finish: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i << " j=" << j);
}

void ignored_dep(Ignore<int&> i) {
  LOG("ignored_dep begin: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i << " addr " << i.addr());
  
  mywait(1.1f);

  LOG("ignored_dep ++" << i);
  i++;
  
  LOG("ignored_dep finish: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i );
}

void ignored_dep2(Ignore<int> i) {
  LOG("ignored_dep2 begin: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i << " addr " << i.addr());
  i++;
  
  LOG("ignored_dep2 finish: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i );
}

int main()
{ int i = 0;//i is in the stack
  
  if (tbb::tbb_thread::hardware_concurrency() < 3 ) {
    std::cout << "TEST SKIPPED (requires 3 HW threads)\n";
    return 0;
  }
  
  std::cout << "i is located at " << (void *)&i << std::endl;
  
  set_threads();
  
  t0 = tbb::tick_count::now();

  spawn(f, i, 1);

  // The changes on i in this g DO change the outside i
  spawn(g, ignore(i), 2);
  // The changes throug an Ignore<int&> are seen outside
  spawn(ignored_dep, i);
  // These changes do not matter because Ignore<int> is a copy, not a reference
  spawn(ignored_dep2, i);
  // The changes on i in this g never change the outside i, so they are pointless
  spawn(h, std::move(i), 200);
  
  wait_for_all();

  std::cout << "Final i=" << i << std::endl;

  const bool test_ok = (i == 4);
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
