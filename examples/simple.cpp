/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2022 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
*/

///
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#include <iostream>
#include "depspawn/depspawn.h"

using namespace depspawn;

void incr(int &i) {
  i++;
}

void add(int &r, int a, int b) {
  r = a + b;
}

int main()
{
  int i = 0, j = 8, r;
  
  set_threads();

  spawn(incr, i);
  spawn(incr, j);
  spawn(add, r, i, j);
  
  wait_for_all();

  std::cout << "Result=" << r << std::endl;

  bool test_ok = (r == 10);
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
