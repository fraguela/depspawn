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

#include <cstdio>
#include <boost/function_types/parameter_types.hpp>

using namespace std;
using namespace boost;

void f(int a[10]) {
  printf("in f -> %lu\n", sizeof(decltype(a)));
}

template<typename T>
void t(T& t) { // <- Only with a reference we get the full array size
  printf("in t -> %lu\n", sizeof(t));
}

void fp(int* a) {
  printf("in fp -> %lu\n", sizeof(a));
}

int main() {
  int a[10];
  int *b = a;
  
  printf("with a (%lu)\n", sizeof(a));
  f(a);
  t(a);
  fp(a);
  
  printf("with b (%lu)\n", sizeof(b));
  f(b);
  t(b);
  fp(b);
  
  printf("analyzing function\n");
  printf("%lu\n", sizeof(mpl::deref<mpl::begin<function_types::parameter_types<decltype(f)>>::type>::type));
  printf("sizeof(int[10]) = %lu\n", sizeof(int[10]));
  
  return 0;
}
