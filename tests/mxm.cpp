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


#define N 1000

int CHUNKS;

Array<Type , 2> result(N,N), a(N, N), b(N,N);

tbb::spin_mutex  my_io_mutex; // This is only for sequencialicing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)

//template<typename Type> //IT DOES NOT WORK WITH A TEMPLATED FUNCTION; AND IT SHOULD!
void mxm(Array<Type , 2>& result, const Array<Type , 2>& a, const Array<Type , 2>& b)
{
  const int nrows = result.rows();
  const int ncols = result.cols();
  const int kdim = a.cols();
  
  for(int i = 0; i < nrows; i++) {
    for(int j = 0; j < ncols; j++) {
      Type f = (Type)0;
      for(int k = 0; k < kdim; k++)
	f += a(i, k) * b(k, j);
      result(i, j) = f;
    }
  }
}

bool test()
{
  const Type *ap = a.data();
  const Type *bp = b.data();
  const Type *rp = result.data();
  
  for (int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      Type f = (Type)0;
      for(int k = 0; k < N; k++)
	f += ap[i * N + k] * bp[k * N + j];
      if (f != rp[i * N + j])
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
    for( j = 0; j < N; j++) {
      result(i, j) = (Type)0;
      a(i, j) = i + j;
      b(i, j) = (i > j) ? i - j : j - i;
    }
  }
  
  tbb::tick_count t0 = tbb::tick_count::now();
  
  mxm(result, a, b);
  
  tbb::tick_count t1 = tbb::tick_count::now();
  
  //matrix - vector product using CHUNKS chunks
  for(i = 0; i < N; i += N / CHUNKS) {
    int limi = (i + N / CHUNKS) >= N ? N : (i + N / CHUNKS);
    Range rows(i, limi - 1);
    for(j = 0; j < N; j += N / CHUNKS) {
      int limj = (j + N / CHUNKS) >= N ? N : (j + N / CHUNKS);
      Range cols(j, limj - 1);
      spawn(mxm, result(rows, cols), a(rows, Range::all()), b(Range::all(), cols));
    }
  }
 
  wait_for_all();
  
  tbb::tick_count t2 = tbb::tick_count::now();
  
  double serial_time = (t1-t0).seconds();
  double parallel_time = (t2-t1).seconds();
  
  std::cout << "Serial time: " << serial_time << "s.  Parallel time: " << parallel_time << "s.\n";
  std::cout << "Speedup (using " << CHUNKS << " X " << CHUNKS << " chunks): " <<  (serial_time / parallel_time) << std::endl;
  
  const bool test_ok = test();
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
