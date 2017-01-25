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

struct t_struct {
  int a, b;
};

t_struct s;

void f(int &i) {
  i++;
}

void g(t_struct& s) {  
  
  s.a++;
}

int main()
{ 
  s.a = 0;
  s.b = 0;
  
  set_threads();
  
  spawn(f, s.a); //Sets s.a to 1
  
  spawn(f, s.b); //Sets s.b to 1

  spawn(g, s);   //Sets s.a to 2
  
  spawn(f, s.a); //Sets s.a to 3
  
  spawn(f, s.b); //Sets s.b to 2
  
  wait_for_all();

  std::cout << "Final s.a =" << s.a << " s.b = " << s.b << std::endl;

  const bool test_ok = ((s.a == 3) && (s.b == 2));
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
