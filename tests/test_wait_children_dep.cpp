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
#include <ctime>
#include <tbb/task_scheduler_init.h>
#include <tbb/tick_count.h>
#include <tbb/spin_mutex.h>  // This is only for serializing parallel prints
#include "depspawn/depspawn.h"

using namespace depspawn;

tbb::spin_mutex  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)


tbb::tick_count t0;

int x;


void mywait(float seconds)
{
  clock_t endwait;
  endwait = clock() + seconds * CLOCKS_PER_SEC ;
  while (clock() < endwait) {}
}

void longf(float f) {
  LOG("longf begin: " << (tbb::tick_count::now() - t0).seconds() << " with f=" << f << " x=" << x);
  
  x = x * 2;
  
  mywait(f);
  
  x = x * 2;
  
  LOG("longf end: " << (tbb::tick_count::now() - t0).seconds() << " with f=" << f << " x=" << x);
}

void f(int &i) {  
  LOG("f begin: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i);
  
  i++;
  i++;
  
  spawn(longf, 4.0f);

  LOG("f finish: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i);
}

void g(int &i) {  
  LOG("g begin: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i);
  
  i++;
  
  LOG("g finish: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i);
}


int main()
{ 
  x = 0;
  
  std::cout << CLOCKS_PER_SEC << std::endl;
  
  tbb::task_scheduler_init tbbinit;
  
  t0 = tbb::tick_count::now();
  
  spawn(f, x); //Sets x to 8
  
  // We see that g does not begin until f and its chil longf finish,
  //even when longf carries not dependencies with g looking at its arguments
  spawn(g, x); //Sets x to 9

  wait_for_all();

  std::cout << "Final x =" <<x << std::endl;

  const bool test_ok = (x == 9);
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
