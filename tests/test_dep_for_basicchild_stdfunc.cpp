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
#include <functional>
#include <tbb/task_scheduler_init.h>
#include <chrono>
#include "depspawn/depspawn.h"
#include "common_io.cpp"  // This is only for serializing parallel prints

using namespace depspawn;

//#define spawn(f,a...) f(a)

//TODO: Cuando F funcione, desmarcar el spawn de g

/////////////////////////////////////////////////////////

#include "common_A.cpp"

/////////////////////////////////////////////////////////

std::chrono::time_point<std::chrono::high_resolution_clock> t0;

bool f_finished, success;

//This function runs for long
void f(int &i) 
{  
  LOG("f begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  
  { // This is just used to make a delay
    A a(400);
    
    a.do_comp();
  }
  
  i = 10;
  
  LOG("f finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i);
  
  f_finished = true;
}

void h(int &i, int s) 
{
  success = f_finished;
  LOG("h begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i);
  std::cout << "h received i=" << i << std::endl;
  i += s;  
  LOG("h finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i);
}  

void g1(int *ptr) 
{
  LOG("g1 begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  
  spawn(h, (*ptr), 10);  //h must wait for the dependency on *ptr, which is seen as it is a reference
  
  LOG("g1 finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
}

void g2(int *ptr) 
{
  LOG("g2 begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  
  std::function<void(int&)> binded_h = std::bind(h, std::placeholders::_1, 10);
  
  //binded_h(*ptr); //Test to see that the function works
  
  spawn(binded_h, (*ptr));
  
  LOG("g2 finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
}


int main()
{ int i;
  
  
  tbb::task_scheduler_init tbbinit;
  
  ////// FIRST TEST USING A REGULAR FUNCTION
  
  i = 0;
  f_finished = success = false;
  
  t0 = std::chrono::high_resolution_clock::now();
  
  //spawn(f, Ignore<int>(i)); //With this version h does not wait for f
  
  spawn(f, i);     //f runs for long. Sets i=10
  
  spawn(g1, &i);   //g1 gets the argument as a pointer, so it should run in parallel as no dependency is detected
  
  wait_for_all();
  
  std::cout << "Final i=" << i << std::endl;
  
  const bool test_ok1 = (i == 20) && success;
  
  std::cout << "TEST REGULAR FUNC " << (test_ok1 ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  ////// SECOND TEST USING A std::function  + std::bind
  
  i = 0;
  f_finished = success = false;
  
  t0 = std::chrono::high_resolution_clock::now();

  //spawn(f, Ignore<int>(i)); //With this version h does not wait for f
  
  spawn(f, i);    //f runs for long. Sets i=10
  
  spawn(g2, &i);  //g2 gets the argument as a pointer, so it should run in parallel as no dependency is detected
  
  wait_for_all();

  std::cout << "Final i=" << i << std::endl;
  
  const bool test_ok2 = (i == 20) && success;
  
  std::cout << "TEST STDFUN + BIND " << (test_ok2 ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !(test_ok1 && test_ok2);
}
