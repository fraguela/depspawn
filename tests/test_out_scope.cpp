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
#include <chrono>
#include "depspawn/depspawn.h"
#include "common_io.cpp"  // This is only for serializing parallel prints

using namespace depspawn;

/////////////////////////////////////////////////////////
// Class used for the test on destruction

class A {
  volatile int n_;
  volatile float *c;
  const char *name_;
  
public:
  
  A(int n, const char *name) 
  : n_(n), c(new float[n *n ]), name_(name)
  {
    LOG("Building " << name_);
    for(int i = 0; i < n_; i++)
      for(int j = 0; j < n_; j++) {
	c[i*n+j] = 0;
      }
				   
  }
  
  /// Copy constructor
  A(const A& other) 
  : n_(other.n_), c(new float[other.n_ * other.n_]), name_(other.name_)
  {
    LOG("Copying " << name_);
    for(int i = 0; i < n_; i++)
      for(int j = 0; j < n_; j++) {
	c[i*n_+j] = other.c[i*n_+j];
      }
  }
  
  /// Move constructor
  A(A&& other)
  : n_(other.n_), c(other.c), name_(other.name_)
  {
    LOG("Moving " << name_);
    other.c = 0;
  }
  
  int size() const { return n_; }
  
  volatile float& operator()(int i, int j)       { return c[i * n_ + j]; }
  
  float           operator()(int i, int j) const { return c[i * n_ + j]; }

  ~A() {
    if(c)
      delete [] c;
    LOG("Destroying " << name_);
  }
  
};

/////////////////////////////////////////////////////////

A a(600, "a");

std::chrono::time_point<std::chrono::high_resolution_clock> t0;

void h(A &a, const A& b) {
  const int s = b.size();
  
  LOG("h " << s << " begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  
  //mywait(0.8f);
  
  a(0, 0) += b(0, 0) + 1;
  a(s-1, s-1) += b(s-1, s-1) + a(s-2, s-2) + 10;
  
  LOG("h " << s << " begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
} 

void g() {
  A b(10, "b");
  
  //spawn(h, a, b); //can break with Segmentation fault because b can be deallocated before h begins to run
  
  spawn(h, a, std::move(b)); //Built once, copied 1 time with T& make(T& t). Destroyed 2 times.
  spawn(h, a, A(11, "c")); //Built once, copied 1 times with T& make(T& t). Destroyed 2 times.

  LOG("g " << b.size() << " finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
}


int main()
{  
  t0 = std::chrono::high_resolution_clock::now();
  
  g();  //Normal call to g. It could also be a spawn, the problem is the same
  
  wait_for_all();
  
  bool test_ok = (a(0,0) == 2) && (a(9, 9) == 10) && (a(10,10) == 20);
  
  spawn(h, std::move(a), a); //a is copied and only the local copy is modified
  
  wait_for_all();
  
  test_ok = test_ok && (a(0,0) == 2) && (a(9, 9) == 10) && (a(10,10) == 20);

  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
