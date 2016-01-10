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
#include <cstdlib>
#include <ctime>
#include <tbb/task_scheduler_init.h>
#include <tbb/tick_count.h>
#include <tbb/spin_mutex.h>  // This is only for serializing parallel prints
#include "depspawn/depspawn.h"

using namespace depspawn;

#define NLEVELS 3

enum TaskState_t { UnRun = 0, Running = 1, Finished = 2 };

tbb::spin_mutex  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)


tbb::tick_count t0;

volatile bool isserial[NLEVELS];
volatile TaskState_t state[NLEVELS];

void mywait(float seconds)
{
  clock_t endwait;
  endwait = clock() + seconds * CLOCKS_PER_SEC ;
  while (clock() < endwait) {}
}

void f(int &i, const int level) 
{
  state[level] = TaskState_t::Running;
  
  LOG("f begin: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i << " level=" << level);


  bool is_serial = true;
  for(int j = 0; j < level; j++)
    is_serial = is_serial && (state[j] == TaskState_t::Finished);
  
  if(level < (NLEVELS-1)) {
    LOG("Spanning f " << (level+1));
    spawn(f, i, level+1);
  }
  
  mywait(2.f);
  
  LOG("f finish: " << (tbb::tick_count::now() - t0).seconds() << " with i=" << i << " level=" << level);
  
  for(int j = level+1; j < NLEVELS; j++)
    is_serial = is_serial && (state[j] == TaskState_t::UnRun);
  isserial[level] = is_serial;
  
  state[level] = TaskState_t::Finished;
}



int main(int argc, char **argv)
{ 
  const int nthreads = (argc == 1) ? 8 : atoi(argv[1]);
  
  tbb::task_scheduler_init tbbinit(nthreads);
  
  std::cout << "Running with " << nthreads << " threads" << std::endl;  
  
  int x = 0;
  
  for(int i = 0; i < NLEVELS; i++)
    state[i] = TaskState_t::UnRun;
  
  t0 = tbb::tick_count::now();
  
  std::cout << "Spanning f 0\n";
  spawn(f, x, 0);

  wait_for_all();

  bool is_serial = true;
  for(int i = 0; i < NLEVELS; i++)
    is_serial = is_serial && isserial[i];
  
  std::cout << "TEST " << ((!is_serial) ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return is_serial;
}
