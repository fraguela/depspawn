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

#include <cstdlib>
#include <iostream>
#include <tbb/task_scheduler_init.h>
#include <tbb/tick_count.h>
#include <tbb/spin_mutex.h>  // This is only for sequencialicing parallel prints
#include <blitz/array.h>
#include "depspawn/depspawn.h"

using namespace depspawn;
using namespace blitz;

typedef float Type;

#define M 6000
#define N 6000

int CHUNKS;

Array<Type , 2> mx(M,N);
Array<Type , 1> result(M), v(N);

tbb::spin_mutex  my_io_mutex; // This is only for sequencialicing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)

void pica(Array<Type , 1>& result)
{
  for(int i = 0; i < result.rows(); i++)
    result(i) = result(i) + (Type)1;
}

void mxv(Array<Type , 1>& result, const Array<Type , 2>& mx, const Array<Type , 1>& v)
{
  const int nrows = mx.rows();
  const int ncols = mx.cols();
  
  for(int i = 0; i < nrows; i++) {
    result(i) = (Type)0;
    for(int j = 0; j < ncols; j++)
      result(i) += mx(i, j) * v(j);
  }
}

bool test()
{
  const Type *mxp = mx.data();
  const Type *vp  = v.data();
  const Type *rp  = result.data();
  
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
  
  CHUNKS = (argc == 1) ? 4 : atoi(argv[1]);
  
  tbb::task_scheduler_init tbbinit;
  
  for (i = 0; i < N; i++) {
    v(i) = i;
  }
  
  for (i = 0; i < M; i++) {
    result(i) = 0;
    for( j = 0; j < N; j++)
      mx(i,j) = i + j;
  }
  
  tbb::tick_count t0 = tbb::tick_count::now();
  
  mxv(result, mx, v);
  
  tbb::tick_count t1 = tbb::tick_count::now();
  
  //matrix - vector product using CHUNKS chunks
  for(i=0; i<M; i+= M / CHUNKS) {
    int lim = (i + M / CHUNKS) >= M ? M : (i + M / CHUNKS);
    Range rows(i, lim -1);
    spawn(mxv, result(rows), mx(rows, Range::all()), v);
    spawn(pica, result(rows));
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
