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
