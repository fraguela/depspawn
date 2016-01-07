/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2016 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 This file is part of DepSpawn.
 
 DepSpawn is free software; you can redistribute it and/or modify it under the terms
 of the GNU General Public License as published by the Free Software Foundation;
 either version 2, or (at your option) any later version.
 
 DepSpawn is distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 
 You should  have received a copy of  the GNU General  Public License along with
 DepSpawn; see the file COPYING.  If not, write to the  Free Software Foundation, 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

///
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <tbb/task_scheduler_init.h>
#include <tbb/spin_mutex.h>  // This is only for sequencialicing parallel prints
#include <blitz/array.h>
#include "depspawn/depspawn.h"

using namespace depspawn;
using namespace blitz;

#define N     30000
#define CHUNK (N/10)

Array<int , 1> r(10), a(N);

std::stringstream Log;
tbb::spin_mutex  my_io_mutex; // This is only for sequencialicing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); Log << __VA_ARGS__ << std::endl; }while(0)

void addreduce(Array<int, 1>& result, const Array<int , 1>& values)
{
  const long long int dest_addr = ((long long int)&result(0));
  const int nchunk = (dest_addr - ((long long int)&r(0))) / sizeof(int);
  
  if( (&values(0)) == (&r(0)) ) {
    LOG("B Final reduction");
  } else {
    LOG("B chk " << nchunk);
  }

  result(0) = 0;
  
  for(int i = 0; i < values.size(); i++)
    result(0) += values(i);
  
  if( (&values(0)) == (&r(0)) ) {
    LOG("Added " << values.size() << " vals->" << result(0) << " . Final");
  } else {
    //LOG("Added " << values.size() << " vals->" << result(0) << " into addr " << dest_addr << " chk" << nchunk);
    LOG("E chk " << nchunk);
  }
}

void print(const Array<int , 1>& v, const char * msg = 0)
{
  if(msg)
    std::cout << msg << " : ";
  std::cout << "[ ";
  for(int i = 0; i < 10; i++)
    std::cout << v(i) << ' ';
  std::cout << "]\n";
}

int main(int argc, char **argv)
{ int i;
  Array<int, 1> result(1);
  
  const int nthreads = (argc == 1) ? 8 : atoi(argv[1]);
  
  tbb::task_scheduler_init tbbinit(nthreads);
  
  std::cout << "Running with " << nthreads << " threads" << std::endl;
  
  for (i = 0; i < N; i++) {
    a(i) = i;
  }
  
  for(i=0; i< N; i+=CHUNK) {
    //LOG("Sp " << (i/10));
    spawn(addreduce, r(Range(i/CHUNK, i/CHUNK)), a(Range(i, i+CHUNK-1)));
  }
  
  LOG("Dependent spawn");
  
  spawn(addreduce, result(Range(0, 0)), r);

  wait_for_all();
 
  //print(a, "First ten values of array a to add");
  //print(r, "Ten partial reductions of ten consecutive elements");
  
  //std::cout << "The addition of the values 0 to 99 is " << result(0) << std::endl;
  //std::cout << "Should be " << 99 * 50 << std::endl;
  
  std::cout << Log.str();
  const bool test_ok = (result(0) == ((N/2)*(N-1)));
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
