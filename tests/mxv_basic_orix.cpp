/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2021 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
*/

///
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#include <cstdlib>
#include <cassert>
#include <iostream>
#include <chrono>
#include "depspawn/depspawn.h"
#include "common_io.cpp"  // This is only for serializing parallel prints

using namespace depspawn;

typedef float Type;

#define M 6000
#define N 6000

//CHUNKS MUST BE A DIVISOR OF M
#define CHUNKS 4

#define BCK (M/CHUNKS)

Type mx[M][N];
Type result[M], v[N];

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

  for (i = 0; i < N; i++) {
    v[i] = i;
  }
  
  for (i = 0; i < M; i++) {
    result[i] = 0;
    for( j = 0; j < N; j++)
      mx[i][j] = i + j;
  }
  
  std::chrono::time_point<std::chrono::high_resolution_clock> t0 = std::chrono::high_resolution_clock::now();
  
  mxv(result, mx, v);
  
  std::chrono::time_point<std::chrono::high_resolution_clock> t1 = std::chrono::high_resolution_clock::now();
  
  //matrix - vector product using CHUNKS chunks
  for(i = 0; i < M; i+= BCK) {
    //mxv(result + i, (const Type (*)[N]) &(mx[i][0]), v);
    spawn(mxv, result + i, (const Type (*)[N]) &(mx[i][0]), v);
    //pica(result + i);
    spawn(pica, result + i);
  }
 
  wait_for_all();
  
  std::chrono::time_point<std::chrono::high_resolution_clock> t2 = std::chrono::high_resolution_clock::now();
  
  double serial_time = std::chrono::duration<double>(t1-t0).count();
  double parallel_time = std::chrono::duration<double>(t2-t1).count();
  
  std::cout << "Serial time: " << serial_time << "s.  Parallel time: " << parallel_time << "s.\n";
  std::cout << "Speedup (using " << CHUNKS << " chunks): " <<  (serial_time / parallel_time) << std::endl;
  
  const bool test_ok = test();
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
