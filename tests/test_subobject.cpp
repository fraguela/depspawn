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
#include <cstdio>
#include <ctime>
#include <string>
#include <tbb/task_scheduler_init.h>
#include <tbb/tick_count.h>
#include <tbb/spin_mutex.h>  // This is only for serializing parallel prints
#include "depspawn/depspawn.h"

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

tbb::spin_mutex  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)


tbb::tick_count t0;

t_struct s;


void mywait(float seconds)
{
  clock_t endwait;
  endwait = clock() + seconds * CLOCKS_PER_SEC ;
  while (clock() < endwait) {}
}

void f(int &i) {  
  LOG("f begin: " << (tbb::tick_count::now() - t0).seconds() << " with s=" << s);
  
  i++;
  
  mywait(2.f);
  
  i++;
  
  LOG("f finish: " << (tbb::tick_count::now() - t0).seconds() << " with s=" << s);
}

void g(t_struct& s) {  
  LOG("g begin: " << (tbb::tick_count::now() - t0).seconds() << " with s=" << s);
  
  s.a++;
  
  mywait(2.f);
  
  s.a++;
  
  LOG("g finish: " << (tbb::tick_count::now() - t0).seconds() << " with s=" << s);
}


int main()
{ 
  s.a = 0;
  s.b = 0;
  
  std::cout << CLOCKS_PER_SEC << std::endl;
  
  tbb::task_scheduler_init tbbinit;
  
  t0 = tbb::tick_count::now();
  
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
