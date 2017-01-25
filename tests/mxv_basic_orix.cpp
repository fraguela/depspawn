/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2017 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
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

#include <cstdlib>
#include <cassert>
#include <iostream>
#include <tbb/task_scheduler_init.h>
#include <tbb/tick_count.h>
#include <tbb/spin_mutex.h>  // This is only for serializing parallel prints
#include "depspawn/depspawn.h"

using namespace depspawn;

typedef float Type;

#define M 6000
#define N 6000

//CHUNKS MUST BE A DIVISOR OF M
#define CHUNKS 4

#define BCK (M/CHUNKS)

Type mx[M][N];
Type result[M], v[N];

tbb::spin_mutex  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)

void pica(Type result[BCK])
{
  for(int i = 0; i < BCK; i++)
    result[i] = result[i] + (Type)1;
}

void mxv(Type result[BCK], const Type mx[M/CHUNKS][N], const Type v[N])
{  
  for(int i = 0; i < BCK; i++) {
    result[i] = (Type)0;
    for(int j = 0; j < N; j++)
      result[i] += mx[i][j] * v[j];
  }
}

bool test()
{
  const Type *mxp = (const Type *)mx;
  const Type *vp  = (const Type *)v;
  const Type *rp  = (const Type *)result;
  
  for (int i = 0; i < M; i++) {
    Type f = (Type)0;
    for(int j = 0; j < N; j++)
      f += mxp[i * N + j] * vp[j];
    if ( (f + (Type)1) != rp[i]) {
      std::cerr << "Error at result " << i << ' ' << (f + (Type)1) << "!=" << rp[i] << std::endl;
      return false;
    }
  }
  
  return true;
}

int main(int argc, char **argv)
{ int i, j;
  
  assert(!(M % CHUNKS));
  
  printf("Using %d chunks\n", CHUNKS);
  
  tbb::task_scheduler_init tbbinit;
  
  for (i = 0; i < N; i++) {
    v[i] = i;
  }
  
  for (i = 0; i < M; i++) {
    result[i] = 0;
    for( j = 0; j < N; j++)
      mx[i][j] = i + j;
  }
  
  tbb::tick_count t0 = tbb::tick_count::now();
  
  mxv(result, mx, v);
  
  tbb::tick_count t1 = tbb::tick_count::now();
  
  //matrix - vector product using CHUNKS chunks
  for(i = 0; i < M; i+= BCK) {
    //mxv(result + i, (const Type (*)[N]) &(mx[i][0]), v);
    spawn(mxv, result + i, (const Type (*)[N]) &(mx[i][0]), v);
    //pica(result + i);
    spawn(pica, result + i);
  }
 
  wait_for_all();
  
  tbb::tick_count t2 = tbb::tick_count::now();
  
  double serial_time = (t1-t0).seconds();
  double parallel_time = (t2-t1).seconds();
  
  std::cout << "Serial time: " << serial_time << "s.  Parallel time: " << parallel_time << "s.\n";
  std::cout << "Speedup (using " << CHUNKS << " chunks): " <<  (serial_time / parallel_time) << std::endl;
  
  const bool test_ok = test();
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
