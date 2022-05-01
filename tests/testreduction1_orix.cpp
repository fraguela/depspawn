/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2022 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
*/

///
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#include <iostream>
#include <blitz/array.h>
#include "depspawn/depspawn.h"
#include "common_io.cpp"  // This is only for serializing parallel prints

using namespace depspawn;
using namespace blitz;

Array<int , 1> r(10), a(100);

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
