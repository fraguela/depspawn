/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2019 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
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
#include <cstdio>
#include <ctime>
#include <string>
#include <chrono>
#include "depspawn/depspawn.h"
#include "common_io.cpp"  // This is only for serializing parallel prints

using namespace depspawn;

struct t_struct {
  int a, b;
  
  std::string string() const {
    char tmp[128];
    sprintf(tmp, "(%d, %d)", a, b);
    return std::string(tmp);
  }
  
};

std::ostream &operator<<(std::ostream &os, const t_struct &a)
{
  os << a.string();
  return os;
}

std::chrono::time_point<std::chrono::high_resolution_clock> t0;

t_struct s;


void mywait(float seconds)
{
  clock_t endwait;
  endwait = clock() + seconds * CLOCKS_PER_SEC ;
  while (clock() < endwait) {}
}

void f(int &i) {  
  LOG("f begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with s=" << s);
  
  i++;
  
  mywait(2.f);
  
  i++;
  
  LOG("f finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with s=" << s);
}

void g(t_struct& s) {  
  LOG("g begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with s=" << s);
  
  s.a++;
  
  mywait(2.f);
  
  s.a++;
  
  LOG("g finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with s=" << s);
}


int main()
{ 
  s.a = 0;
  s.b = 0;
  
  std::cout << CLOCKS_PER_SEC << std::endl;
  
  set_threads();
  
  t0 = std::chrono::high_resolution_clock::now();
  
  spawn(f, s.a); //Sets s.a to 2
  
  spawn(f, s.b); //Sets s.b to 2

  spawn(g, s); //Sets s.a to 4
  
  spawn(f, s.a); //Sets s.a to 6
  
  spawn(f, s.b); //Sets s.b to 4
  
  wait_for_all();

  std::cout << "Final s.a =" << s.a << " s.b = " << s.b << std::endl;

  const bool test_ok = ((s.a == 6) && (s.b == 4));
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
