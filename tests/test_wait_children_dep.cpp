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
#include <tbb/task_scheduler_init.h>
#include <tbb/tick_count.h>
#include <tbb/spin_mutex.h>  // This is only for serializing parallel prints
#include "depspawn/depspawn.h"

using namespace depspawn;

tbb::spin_mutex  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)


tbb::tick_count t0;

int x;


void mywait(float seconds)
{
  clock_t endwait;
  endwait = clock() + seconds * CLOCKS_PER_SEC ;
  while (clock() < endwait) {}
}

void longf(float f) {
  LOG("longf begin: " << (tbb::tick_count::now() - t0).seconds() << " with f=" << f << " x=" << x);
  
  x = x * 2;
  
  mywait(f);
  
  x = x * 2;
  
  LOG("longf end: " << (tbb::tick_count::now() - t0).seconds() << " with f=" << f << " x=" << x);
}

void f(int &i) {  
  LOG("f begin: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i);
  
  i++;
  i++;
  
  spawn(longf, 4.0f);

  LOG("f finish: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i);
}

void g(int &i) {  
  LOG("g begin: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i);
  
  i++;
  
  LOG("g finish: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i);
}


int main()
{ 
  x = 0;
  
  std::cout << CLOCKS_PER_SEC << std::endl;
  
  tbb::task_scheduler_init tbbinit;
  
  t0 = tbb::tick_count::now();
  
  spawn(f, x); //Sets x to 8
  
  // We see that g does not begin until f and its chil longf finish,
  //even when longf carries not dependencies with g looking at its arguments
  spawn(g, x); //Sets x to 9

  wait_for_all();

  std::cout << "Final x =" <<x << std::endl;

  const bool test_ok = (x == 9);
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
