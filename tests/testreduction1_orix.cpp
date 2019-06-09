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
#include <tbb/task_scheduler_init.h>
#include <tbb/spin_mutex.h>  // This is only for serializing parallel prints
#include <blitz/array.h>
#include "depspawn/depspawn.h"

using namespace depspawn;
using namespace blitz;

Array<int , 1> r(10), a(100);

tbb::spin_mutex  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)

void addreduce(int& result, const Array<int , 1>& values)
{
  result = 0;
  
  for(int i = 0; i < values.size(); i++)
    result += values(i);
  
  LOG("Added " << values.size() << " values -> " << result << " into address " << ((long int)&result));
}


void print(const Array<int , 1>& v, const char * msg = 0)
{
  if(msg)
    std::cout << msg << " : ";
  std::cout << "[ ";
  for(int i = 0; i < 10; i++)
    std::cout << v(i) << ' ' /**/ << ((long int)&(v(i))) << ' ' /**/;
  std::cout << "]\n";
}

int main()
{ int i, result;
  
  tbb::task_scheduler_init tbbinit;
  
  for (i = 0; i < 100; i++) {
    a(i) = i;
  }
  
  for(i=0; i<100; i+=10) {
    LOG("iter " << i);
    spawn(addreduce, r(i/10), a(Range(i, i+9)));
  }
  
  spawn(addreduce, result, r);
 
  wait_for_all();
  
  print(a,"First ten values of array a to add");
  print(r, "Ten partial reductions of ten consecutive elements");
  
  std::cout << "The addition of the values 0 to 99 is " << result << " and should be " << 99 * 50 << std::endl;
  
  return 0;
}
