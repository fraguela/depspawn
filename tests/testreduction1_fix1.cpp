/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2023 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
*/

///
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <blitz/array.h>
#include "depspawn/depspawn.h"
#include "common_io.cpp"  // This is only for serializing parallel prints

using namespace depspawn;
using namespace blitz;

#define N     30000
#define CHUNK (N/10)

Array<int , 1> r(10), a(N);

std::stringstream Log;

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
  
  set_threads(nthreads);
  
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
