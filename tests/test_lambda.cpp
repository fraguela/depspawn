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
#include <functional>
#include <tbb/task_scheduler_init.h>
#include <tbb/spin_mutex.h>  // This is only for sequencialicing parallel prints
#include "depspawn/depspawn.h"

using namespace depspawn;

tbb::spin_mutex  my_io_mutex; // This is only for sequencialicing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)

int i = 0, tmpresult = 0;

struct X
{
  const int v_;
  
  X(int v=0)
  : v_(v)
  {}
  
  bool f(int a) {
    return a == v_;
  }

  void operator() (int& a) {
    std::cout << "Op " << a << " += " << v_ << std::endl;
    a += v_;
  }
 
  void operator() (int& a, int b) {
    std::cout << "Op " << a << " += " << v_ << " + " << b << std::endl;
    a += v_ + b;
  }
};

int main()
{

  //in_stack(i);
  //in_stack(tmpresult);
  
  tbb::task_scheduler_init tbbinit;
  
  spawn([](int &r, const int& i) { r = i + 1; LOG("setting " << r); }, tmpresult, 99);
  
  spawn([](int &r, const int& i) { r = 2 * i; LOG("setting " << r); }, i, tmpresult);
  
  wait_for_all();

  const bool test_ok1 = (tmpresult == 100) && (i == 200);
  std::cout << "LAMBDA TEST " << (test_ok1 ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  

  wait_for_all();
 
  X x(10);
  
  // Break :
  // spawn(&X::f, x, i);
  // spawn(std::mem_fn(&X::f), x, i);
  // spawn(x, i);
  
  std::function<void(int&)> f = x;
  std::function<void(int&, int)> g = x;
  spawn(f, i);
  spawn(f, tmpresult);
  spawn(g, i, tmpresult);
  
  wait_for_all();
  
  const bool test_ok2 = (tmpresult == 110) && (i == 330);
  std::cout << "FUNCTOR TEST " << (test_ok1 ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !(test_ok1 && test_ok2);
}
