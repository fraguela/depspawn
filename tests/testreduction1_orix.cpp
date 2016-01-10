/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2016 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 This file is part of DepSpawn.
 
 DepSpawn is free software; you can redistribute it and/or modify it under the terms
 of the GNU General Public License as published by the Free Software Foundation;
 either version 2 as published by the Free Software Foundation.
 
 DepSpawn is distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with Threading Building Blocks; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 As a special exception, you may use this file as part of a free software
 library without restriction.  Specifically, if other files instantiate
 templates or use macros or inline functions from this file, or you compile
 this file and link it with other files to produce an executable, this
 file does not by itself cause the resulting executable to be covered by
 the GNU General Public License.  This exception does not however
 invalidate any other reasons why the executable file might be covered by
 the GNU General Public License.
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
