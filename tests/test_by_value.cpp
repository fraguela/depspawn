/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2023 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
*/

///
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#include <ctime>
#include <iostream>
#include <thread>
#include <chrono>
#include "depspawn/depspawn.h"
#include "common_io.cpp"  // This is only for serializing parallel prints

using namespace depspawn;

std::chrono::time_point<std::chrono::high_resolution_clock> t0;

int i = 0, j = 0;

void mywait(float seconds)
{
  clock_t endwait;
  endwait = clock() + seconds * CLOCKS_PER_SEC ;
  while (clock() < endwait) {}
}

//This function runs for long
void f(volatile int &i) {
  LOG("f begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  
  // This is just used to make a delay
  mywait(1.0);
  
  while (!i) {
    /* wait */
  }
  
  i = 10;
  
  LOG("f finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i);
}

void h(int &i, int s) {
  LOG("h begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i << " s=" << s );
  
  i += s;  
  
  LOG("h finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i);
}

int main()
{
  
  if (std::thread::hardware_concurrency() < 2 ) {
    std::cout << "TEST SKIPPED (requires 2 HW threads)\n";
    return 0;
  }

  t0 = std::chrono::high_resolution_clock::now();

  spawn(f, i);      //f runs for long. Sets i=10
  
  i = 20;
  
  // Because of the use of std::move(i)
  // (1) h does not depend on f and thus does not wait for it
  // (2) h uses the inmediate value of i(20)
  spawn(h, j, std::move(i));
  
  // Although i was set to 20, f sets it to 10 much later
  // h proves that although the second argument (s) is passed by value,
  // it is binded/provided to the function ONLY after f finishes, thus
  // adquiring the value 10, not 20
  spawn(h, i, i);
  
  wait_for_all();

  std::cout << "Final i=" << i << " j=" << j << std::endl;
  
  const bool test_ok = (i == 20) && (j == 20);
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
