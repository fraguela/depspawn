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

#include <ctime>
#include <iostream>
#include <functional>
#include <tbb/task_scheduler_init.h>
#include <tbb/spin_mutex.h>  // This is only for serializing parallel prints
#include "depspawn/depspawn.h"

using namespace depspawn;

tbb::spin_mutex  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)

int i = 0, tmpresult = 0;

void mywait(float seconds)
{
  clock_t endwait;
  endwait = clock() + seconds * CLOCKS_PER_SEC ;
  while (clock() < endwait) {}
}

struct X
{
  const int v_;
  
  X(int v=0)
  : v_(v)
  {}
  
  void sub(int& a) const {
    mywait(1.f);
    a -= v_;
  }

  void operator() (int& a) const {
    mywait(1.f);
    std::cout << "Op " << a << " += " << v_ << std::endl;
    a += v_;
  }
 
  void operator() (int& a, int b) {
    mywait(1.f);
    std::cout << "Op " << a << " += " << v_ << " + " << b << std::endl;
    a += v_ + b;
  }
};

class SingleFunctor {

  int v_;

public:
  
  SingleFunctor(int v) :
  v_(v)
  {}
  
  int value() const { return v_; }
  
  void operator() (int& i) {
    mywait(1.f);
    v_ += i;
    i++;
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
  
  X x(10);
  
  std::function<void(int&)> f = x;
  std::function<void(int&, int)> g = x;
  
  spawn(f, i);            // i = 200 + 10 = 210
  spawn(f, tmpresult);    // tmpresult = 100 + 10 = 110
  spawn(g, i, tmpresult); // i = 210 + 10 + 110 = 330
  
  wait_for_all();
  
  const bool test_ok2 = (tmpresult == 110) && (i == 330);
  std::cout << "STD::FUNCTION TEST " << (test_ok2 ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  const X y(5);
  
  void (X::*ptrmem)(int&) const;
  ptrmem = &X::sub;
  
  spawn(&X::sub, x, i);          // i = 330 - 10 = 320
  spawn(ptrmem, y, tmpresult);   // tmpresult = 110 - 5 = 105
  ptrmem = &X::operator();
  spawn(ptrmem, y, i);           // i = 320 + 5 = 325;
  
  wait_for_all();
  
  const bool test_ok3 = (tmpresult == 105) && (i == 325);
  std::cout << "PTR TO MEMBER FUNCTION TEST " << (test_ok3 ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  SingleFunctor sf1(100);
  spawn(sf1, i);         //sf1.v_ = 100 + 325 = 425;  i++= 326
  spawn(sf1, tmpresult); //sf1.v_ = 425 + 105 = 530;  tmpresult++= 106
  spawn(sf1, i);         //sf1.v_ = 530 + 326 = 856;  i++= 327
  wait_for_all();
  
  const bool test_ok4 = (tmpresult == 106) && (i == 327) && (sf1.value() == 856);
  std::cout << "FUNCTOR TEST " << (test_ok4 ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !(test_ok1 && test_ok2 && test_ok3);
}
