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

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <tbb/task_scheduler_init.h>
#include <chrono>
#include "depspawn/depspawn.h"
#include "common_io.cpp"  // This is only for serializing parallel prints

using namespace depspawn;

#define NLEVELS 3

enum TaskState_t { UnRun = 0, Running = 1, Finished = 2 };

std::chrono::time_point<std::chrono::high_resolution_clock> t0;

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
  
  LOG("f begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i << " level=" << level);


  bool is_serial = true;
  for(int j = 0; j < level; j++)
    is_serial = is_serial && (state[j] == TaskState_t::Finished);
  
  if(level < (NLEVELS-1)) {
    LOG("Spanning f " << (level+1));
    spawn(f, i, level+1);
  }
  
  mywait(2.f);
  
  LOG("f finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i << " level=" << level);
  
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
  
  t0 = std::chrono::high_resolution_clock::now();
  
  std::cout << "Spanning f 0\n";
  spawn(f, x, 0);

  wait_for_all();

  bool is_serial = true;
  for(int i = 0; i < NLEVELS; i++)
    is_serial = is_serial && isserial[i];
  
  std::cout << "TEST " << ((!is_serial) ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return is_serial;
}
