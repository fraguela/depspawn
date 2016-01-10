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
#include <tbb/compat/thread>
#include <tbb/tick_count.h>
#include <tbb/spin_mutex.h>  // This is only for serializing parallel prints
#include "depspawn/depspawn.h"

using namespace depspawn;

tbb::spin_mutex  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)

tbb::tick_count t0;

int i = 0, j = 0;

void mywait(float seconds)
{
  clock_t endwait;
  endwait = clock() + seconds * CLOCKS_PER_SEC ;
  while (clock() < endwait) {}
}

//This function runs for long
void f(volatile int &i) {
  LOG("f begin: " << (tbb::tick_count::now() - t0).seconds());
  
  // This is just used to make a delay
  mywait(1.0);
  
  while (!i) {
    /* wait */
  }
  
  i = 10;
  
  LOG("f finish: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i);
}

void h(int &i, int s) {
  LOG("h begin: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i << " s=" << s );
  
  i += s;  
  
  LOG("h finish: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i);
}

int main()
{
  
  if (tbb::tbb_thread::hardware_concurrency() < 2 ) {
    std::cout << "TEST SKIPPED (requires 2 HW threads)\n";
    return 0;
  }
  
  set_threads();
  
  t0 = tbb::tick_count::now();

  spawn(f, i);      //f runs for long. Sets i=10
  
  i = 20;
  
  // Because of the use of std::move(i)
  // (1) h does not depend on f and thus does not wait for it
  // (2) h uses the inmediate value of i(20)
  spawn(h, j, std::move(i));
  
  // Although i was set to 20, f sets it to 10 much later
  // h proves that although the second argument (s) is passed by value,
  // it is binded/provided to the function ONLY after f finishes, thus
  // adquiring the value 10, not 20
  spawn(h, i, i);
  
  wait_for_all();

  std::cout << "Final i=" << i << " j=" << j << std::endl;
  
  const bool test_ok = (i == 20) && (j == 20);
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
