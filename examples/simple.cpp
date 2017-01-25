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
