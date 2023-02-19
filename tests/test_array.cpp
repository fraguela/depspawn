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
//#include <algorithm>
#include <chrono>
#include <atomic>
#include "common_io.cpp"  // This is only for serializing parallel prints

#include "depspawn/depspawn.h"

using namespace boost;
using namespace depspawn;

typedef int ct22[22];

volatile bool inwritemode = false;

std::chrono::time_point<std::chrono::high_resolution_clock> t0;

bool second_round = false;

std::atomic<int> x, success_counter;

void fun(bool readermode, int n = 600) {
  volatile double a = 2.1, b = 3.2, c=-42.1;
  
  if(readermode && inwritemode) LOG("ERROR!");
  else success_counter++;
  
  for(int i = 0; i < n; i++)
    for(int j = 0; j < n; j++)
      for(int k = 0; k < n; k++)
        a = a*b/c;
  if(readermode && inwritemode) LOG("ERROR!");
  else success_counter++;
}

// Because the input is a pointer, the region pointed is not tracked and, in theory it can be read or written
//in parallel with other reads or writes. In practice, it can be read or written in parallel with other reads
//but not with writes because the whole input array is used in the invocation and it is tracked as a "read"
//because the pointer is just read, as it is passed by value. Same in f5.
void f1(int a[10])
{ 
  LOG("f1 " << a[0] << " begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count()); 
  a[0]++;
  fun(true);
  LOG("f1 " << a[0] << " end: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  x.fetch_add(1);
}

void f2(const int (&a)[22])
{ 
  success_counter.fetch_add((second_round && (x<0)) || (!second_round && (x>=0)));
  
  LOG("f2 " << a[0] << " begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count()); 
  //a[0]++;
  fun(true);
  LOG("f2 " << a[0] << " end: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  x.fetch_add(8);
  success_counter.fetch_add((second_round && (x<0)) || (!second_round && (x>=0)));
}

void f2b(const int (&a)[22])
{ 
  success_counter.fetch_add((second_round && (x<0)) || (!second_round && (x>=0)));
  LOG("f2b " << a[0] << " begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  //a[0]++;
  fun(true);
  LOG("f2b " << a[0] << " end: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  x.fetch_add(16);
  success_counter.fetch_add((second_round && (x<0)) || (!second_round && (x>=0)));
}

void f2c(const ct22& a)
{ 
  success_counter.fetch_add((second_round && (x<0)) || (!second_round && (x>=0)));
  LOG("f2c " << a[0] << " begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count()); 
  //a[0]++;
  fun(true);
  LOG("f2c " << a[0] << " end: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  x.fetch_add(32);
  success_counter.fetch_add((second_round && (x<0)) || (!second_round && (x>=0)));
}

void f3(ct22& a)
{ 
  success_counter.fetch_add(x >= (32 + 16 + 8)); //f2, f2b and f2c must have finished
  inwritemode = true;
  LOG("-------------\nf3 " << a[0] << " begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  a[0]++;
  fun(false);
  LOG("f3 " << a[0] << " end: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  inwritemode = false;
  x.fetch_add(-(32 + 16 + 8));
}

void f3b(int (&a)[22])
{ 
  success_counter.fetch_add((x >= 0) && (x < 8)); //f3 must have finished
  inwritemode = true;
  LOG("f3b " << a[0] << " begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  a[0]++;
  fun(false);
  LOG("f3b " << a[0] << " end: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << "\n-------------");
  inwritemode = false;
  x = -256;
  second_round = true;
}

void f5(int a[10]) //identical to f1. Just different name
{ 
  LOG("f5 " << a[0] << " begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count()); 
  fun(true);
  LOG("f5 " << a[0] << " end: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  x.fetch_add(2);
}
  
int main(int argc, char **argv) 
{ int a[22], tmp;
  
  a[0] = 0;
  
  const int nthreads = (argc == 1) ? 8 : atoi(argv[1]);
  
  set_threads(nthreads);
  
  std::cout << "Running with " << nthreads << " threads" << std::endl;
  
  x = 0;
  success_counter = 0;
  
  t0 = std::chrono::high_resolution_clock::now();
  
  spawn(f1, a);
  
  spawn(f2, a);
  
  spawn(f2b, a);
  
  spawn(f2c, a);
  
  // Write
  spawn(f3, a);
  
  //Write
  spawn(f3b, a);
  
  spawn(f5, a);
  
  spawn(f1, a);
  
  spawn(f2, a);
  
  spawn(f2b, a);
  
  spawn(f2c, a);
  
  wait_for_all();
  
  tmp = success_counter; //11 f * 2 + 6 * 2 + 2 = 36
  
  const bool test_ok = (tmp == 36);

  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}

