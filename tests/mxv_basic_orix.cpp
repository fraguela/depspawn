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
