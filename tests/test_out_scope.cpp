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
#include <tbb/tick_count.h>
#include <tbb/spin_mutex.h>  // This is only for serializing parallel prints
#include "depspawn/depspawn.h"

using namespace depspawn;

tbb::spin_mutex  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)

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

tbb::tick_count t0;

void h(A &a, const A& b) {
  const int s = b.size();
  
  LOG("h " << s << " begin: " << (tbb::tick_count::now() - t0).seconds());
  
  //mywait(0.8f);
  
  a(0, 0) += b(0, 0) + 1;
  a(s-1, s-1) += b(s-1, s-1) + a(s-2, s-2) + 10;
  
  LOG("h " << s << " begin: " << (tbb::tick_count::now() - t0).seconds());
} 

void g() {
  A b(10, "b");
  
  //spawn(h, a, b); //can break with Segmentation fault because b can be deallocated before h begins to run
  
  spawn(h, a, std::move(b)); //Built once, copied 1 time with T& make(T& t). Destroyed 2 times.
  spawn(h, a, A(11, "c")); //Built once, copied 1 times with T& make(T& t). Destroyed 2 times.

  LOG("g " << b.size() << " finish: " << (tbb::tick_count::now() - t0).seconds());
}


int main()
{
  tbb::task_scheduler_init tbbinit;
  
  t0 = tbb::tick_count::now();
  
  g();  //Normal call to g. It could also be a spawn, the problem is the same
  
  wait_for_all();
  
  bool test_ok = (a(0,0) == 2) && (a(9, 9) == 10) && (a(10,10) == 20);
  
  spawn(h, std::move(a), a); //a is copied and only the local copy is modified
  
  wait_for_all();
  
  test_ok = test_ok && (a(0,0) == 2) && (a(9, 9) == 10) && (a(10,10) == 20);

  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
