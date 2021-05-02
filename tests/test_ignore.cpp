/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2021 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
*/

///
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#include <iostream>
#include <ctime>
#include <thread>
#include <chrono>
#include "depspawn/depspawn.h"
#include "common_io.cpp"  // This is only for serializing parallel prints

using namespace depspawn;

std::chrono::time_point<std::chrono::high_resolution_clock> t0;

void mywait(float seconds)
{
  clock_t endwait;
  endwait = clock() + seconds * CLOCKS_PER_SEC ;
  while (clock() < endwait) {}
}

void f(volatile int &i, int j) {
  LOG("f begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i << " j=" << j);
  
  mywait(1.5f);
  
  LOG("f comes back at: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i << " j=" << j);
  
  while (i != 3) {
    //g should be able to run before this point of f, and thus let i=3
  }
  
  i += j;
  
  LOG("f finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i << " j=" << j);
}

void g(volatile int &i, int j) {
  LOG("g begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i << " j=" << j);
  
  while(!i) {
    //ignored_dep should be able to run before this point, letting i==1
  }
  i += j;
  
  LOG("g finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i << " j=" << j);
}

void h(volatile int &i, int j) {
  LOG("h begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i << " j=" << j);
  

  i += j;
  
  LOG("h finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i << " j=" << j);
}

void ignored_dep(Ignore<int&> i) {
  LOG("ignored_dep begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i << " addr " << i.addr());
  
  mywait(1.1f);

  LOG("ignored_dep ++" << i);
  i++;
  
  LOG("ignored_dep finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i );
}

void ignored_dep2(Ignore<int> i) {
  LOG("ignored_dep2 begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i << " addr " << i.addr());
  i++;
  
  LOG("ignored_dep2 finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i );
}

int main()
{ int i = 0;//i is in the stack
  
  if (std::thread::hardware_concurrency() < 3 ) {
    std::cout << "TEST SKIPPED (requires 3 HW threads)\n";
    return 0;
  }
  
  std::cout << "i is located at " << (void *)&i << std::endl;

  t0 = std::chrono::high_resolution_clock::now();

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
