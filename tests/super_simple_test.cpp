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
#include <chrono>
#include "depspawn/depspawn.h"
#include "common_io.cpp"  // This is only for serializing parallel prints

using namespace depspawn;

std::chrono::time_point<std::chrono::high_resolution_clock> t0;

void f(int &i) {  
  LOG("f begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());

  i = 10;
  
  LOG("f finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i);
}


int main()
{ int i = 0;
  
  set_threads();

  t0 = std::chrono::high_resolution_clock::now();
  
  spawn(f, i);
  
  wait_for_all();

  std::cout << "Final i=" << i << std::endl;

  const bool test_ok = (i == 10);
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
