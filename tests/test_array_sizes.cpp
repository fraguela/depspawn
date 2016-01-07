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
