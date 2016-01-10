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
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <tbb/task_scheduler_init.h>
#include <tbb/tick_count.h>
#include <tbb/spin_mutex.h>  // This is only for serializing parallel prints
#include "depspawn/depspawn.h"

using namespace depspawn;


tbb::spin_mutex  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)

//#define spawn(f,a...) f(a)

//TODO: Cuando F funcione, desmarcar el spawn de g

/////////////////////////////////////////////////////////

#include "common_A.cpp"

/////////////////////////////////////////////////////////

tbb::tick_count t0;


//This function runs for long
void f(int &i) {  
  LOG("f begin: " << (tbb::tick_count::now() - t0).seconds());
  
  { // This is just used to make a delay
    A a(400);
  
    a.do_comp();
  }
  
  i = 10;
  
  LOG("f finish: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i);
}

void h(int &i, int s) {
  LOG("h begin: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i << " +=" << s);
  
  i += s;  
  
  LOG("h finish: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i);
} 

void g(int *ptr) {
  LOG("g begin: " << (tbb::tick_count::now() - t0).seconds());
  
  boost::function<void(int&)> binded_h = boost::bind(h, _1, 10);
  
  //binded_h(*ptr); //Test to see that the function works
  
  spawn(binded_h, (*ptr));
  
  LOG("g finish: " << (tbb::tick_count::now() - t0).seconds());
}


int main()
{ int i = 0; //i is in the stack
  
  tbb::task_scheduler_init tbbinit;
  
  t0 = tbb::tick_count::now();

  //spawn(f, Ignore<int>(i)); //With this version h does not wait for f
  
  spawn(f, i);      //f runs for long. Sets i=10
  
  spawn(g, (&i));   //g gets the argument as a pointer, so it should run in parallel as no dependency is detected
  
  wait_for_all();

  std::cout << "Final i=" << i << std::endl;
  
  const bool test_ok = (i == 20);
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
