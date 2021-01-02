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
#include <ctime>
#include <chrono>
#include "depspawn/depspawn.h"
#include "common_io.cpp"  // This is only for serializing parallel prints

using namespace depspawn;

std::chrono::time_point<std::chrono::high_resolution_clock> t0;

int x;


void mywait(float seconds)
{
  clock_t endwait;
  endwait = clock() + seconds * CLOCKS_PER_SEC ;
  while (clock() < endwait) {}
}

void longf(float f) {
  LOG("longf begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with f=" << f << " x=" << x);
  
  x = x * 2;
  
  mywait(f);
  
  x = x * 2;
  
  LOG("longf end: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with f=" << f << " x=" << x);
}

void f(int &i) {  
  LOG("f begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i);
  
  i++;
  
  spawn(longf, 1.5f);

  LOG("f finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i);
}

void g(int &i) {  
  LOG("g begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i);
  
  i++;
  
  LOG("g finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i);
}

void h(int &i, bool& test_ok) {
  LOG("h begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i);
  
  i = 0;
  
  { Observer o;
    
    spawn(g, i); //Sets x to 1
    spawn(g, i); //Sets x to 2
    spawn(f, i); //Sets x to 12
  }
  
  test_ok = test_ok && (i == 12);
  std::cout << "x = " << x << std::endl;
  
  LOG("h finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i);
}

int main()
{ 
  x = 0;
  
  std::cout << CLOCKS_PER_SEC << std::endl;
  
  set_threads();
  
  t0 = std::chrono::high_resolution_clock::now();
  
  { Observer o;
    
    spawn(g, x); //Sets x to 1
    spawn(g, x); //Sets x to 2
    spawn(f, x); //Sets x to 12
  }

  std::cout << "x = " << x << std::endl;

  bool test_ok = (x == 12);

  { Observer o;
    
    spawn(h, x, test_ok);
  }
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
