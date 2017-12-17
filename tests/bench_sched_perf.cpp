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

/// \file     bench_sched_perf.cpp
/// \brief    Tests scheduling performance
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>

#include <cstring>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include "depspawn/depspawn.h"
#include <tbb/parallel_for.h>

using namespace depspawn;

constexpr size_t MAX_TILE_SZ = 100;
constexpr size_t MAX_NTASKS = (1 << 14);

struct Tile {
  double data_[MAX_TILE_SZ][MAX_TILE_SZ];
  
  void clear() {
    memset((void *)data_, 0, sizeof(data_));
  }
};

int Nthreads = 4;
int NReps = 1;
size_t MaxTileSize = MAX_TILE_SZ;
size_t MaxNTasks = MAX_NTASKS;
size_t cur_tile_sz;
bool UseSameTile = false;
bool ClearCaches = false;
bool OnlyParallelRun = false; // This is for profiling
int Queue_limit = -1;
int Verbosity = 0;
Tile Input1, Input2, Destination[MAX_NTASKS];
std::vector<double> TSeq, TPar;

void clearDestination()
{
  for (size_t i = 0; i < MAX_NTASKS; i++) {
    Destination[i].clear();
  }
}

// I tried working on a local tmp tile instead of on dest.data in order to avoid
//the extra memory cost of the remote access in NUMA machines. But the compiler
//detected that the output was not written and skipped the computations.
// input1 and input2 should be fine because they are a single read-only tile.
void seq_mult(Tile& dest, const Tile& input1, const Tile& input2)
{
  for (size_t i = 0; i < cur_tile_sz; i++) {
    for (size_t j = 0; j < cur_tile_sz; j++) {
      for (size_t k = 0; k < cur_tile_sz; k++) {
        dest.data_[i][j] += input1.data_[i][k] * input2.data_[k][j];
      }
    }
  }
}

double bench_serial_time(const size_t ntasks, int nreps)
{ double ret_time = 0.;
  
  //upcxx::barrier();
  
  for (int nr = 0; nr < nreps; nr++) {
    
    if (ClearCaches) {
      clearDestination();
    }
    
    std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < ntasks; i++) {
      seq_mult(Destination[UseSameTile ? 0 : i], Input1, Input2);
    }
    
    TSeq[nr] =  std::chrono::duration <double>(std::chrono::high_resolution_clock::now() - t0).count();
    ret_time += TSeq[nr];
  }
  
  return ret_time;
}

double bench_parallel_time(const size_t ntasks, int nreps)
{ double ret_time = 0.;
  
  for (int nr = 0; nr < nreps; nr++) {
    
    if (ClearCaches) {
      clearDestination();
    }
    
    std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < ntasks; i++) {
      spawn(seq_mult, Destination[UseSameTile ? 0 : i], Input1, Input2);
    }
    wait_for_all();
    
    TPar[nr] =  std::chrono::duration <double>(std::chrono::high_resolution_clock::now() - t0).count();
    ret_time += TPar[nr];
  }
  
  return ret_time;
}

double bench_sched_perf(const size_t ntasks)
{ double serial_time = 0.;
  
  if (!OnlyParallelRun) {
    
    // heat the caches if they are not going to be cleaned.
    //It is not done in profiling runs because of the runtime impact
    if (!ClearCaches) {
      bench_parallel_time(ntasks, 1);
    }
    
    serial_time = bench_serial_time(ntasks, NReps);
    if(Verbosity) {
      if (Verbosity > 1) {
        for (double t : TSeq) {
          std::cout << std::setw(8) << t << ' ';
        }
      }
      std::cout << "  serial time=" << serial_time << std::endl;
    }
  }

  const double parallel_time = bench_parallel_time(ntasks, NReps);
  if(Verbosity) {
    if (Verbosity > 1) {
      for (double t : TPar) {
        std::cout << std::setw(8) << t << ' ';
      }
    }
    std::cout << "parallel time=" << parallel_time << std::endl;
  }
  
  return OnlyParallelRun ? 1.0 : (parallel_time / (serial_time / (UseSameTile ? 1 : Nthreads)));
}

void show_help()
{
  puts("bench_sched_perf [-c] [-h] [-n reps] [-p] [-q limit] [-s] [-t nthreads] [-T maxtilesize] [-v level]");
  puts("-c          clear caches before test");
  puts("-h          Display help and exit");
  puts("-N ntasks   Maximum number of tasks");
  puts("-n reps     Repeat each measurement reps times");
  puts("-p          Only parallel run (for profiling)");
  puts("-q limit    queue limit");
  puts("-s          Work always on same tile (no parallelism)");
  puts("-T tilesz   Maximum tile size");
  puts("-t nthreads Run with nthreads threads. Default is 4");
  puts("-v level    Verbosity level");
}

void process_arguments(int argc, char **argv)
{ int c;

  while ( -1 != (c = getopt(argc, argv, "chN:n:pq:sT:t:v:")) ) {
    switch (c) {
      case 'c':
        ClearCaches = true;
        break;
      case 'h':
        show_help();
        exit(0);
        break;
      case 'N':
        c = strtoul(optarg, 0, 0);
        if (c < MAX_NTASKS) {
          MaxNTasks = c;
        }
        break;
      case 'n':
        NReps = strtoul(optarg, 0, 0);
        break;
      case 'p':
        OnlyParallelRun = true;
        break;
      case 'q' : /* queue limit */
        Queue_limit = strtoul(optarg, 0, 0);
        break;
      case 's':
        UseSameTile = true;
        break;
      case 'T':
        MaxTileSize = std::min(strtoul(optarg, 0, 0), MAX_NTASKS);
        break;
      case 't':
        Nthreads = strtoul(optarg, 0, 0);
        break;
      case 'v':
        Verbosity = strtoul(optarg, 0, 0);
        break;
      default:
        fprintf(stderr, "Unknown argument %c. Try -h\n", (char)c);
    }
  }
}

int main(int argc, char **argv)
{
  process_arguments(argc, argv);
  
  set_threads(Nthreads);

  if (Queue_limit >= 0) {
    set_task_queue_limit(Queue_limit);
  }
  
  TSeq.resize(NReps);
  TPar.resize(NReps);

  std::cout << Nthreads << " threads max_tile_size=" << MaxTileSize << " NReps=" << NReps << " ClearCache=" << (ClearCaches ? 'Y' : 'N') << " QueueLimit=" << Queue_limit << " EnqueueTasks=" << (depspawn::internal::EnqueueTasks ? 'Y' : 'N') << std::endl;
  
  if(!OnlyParallelRun) {
    // This is to try to build the threads before the first test; just in case
    tbb::parallel_for(size_t(0), MAX_NTASKS, [](size_t i) {
      Destination[i].clear();
    });

    //1-time runtime preheat (for memory pools)
    bench_parallel_time(MaxNTasks, 1);
  }
  
  std::cout << "size %";
  for (size_t ntasks = (1 << 7); ntasks <= MaxNTasks; ntasks *= 2) {
    std::cout << std::setw(5) << ntasks << "    ";
  }
  std::cout << "tasks\n";
  
  for (size_t tile_sz=10; tile_sz <= MaxTileSize; tile_sz += 5) {
    if (Verbosity) {
      std::cout << "Tests for TSZ=";
    }
    std::cout << std::right << std::setw(3) << tile_sz << ' ' << std::left;
    if (Verbosity) {
      std::cout << std::endl;
    }
    cur_tile_sz = tile_sz;
    for (size_t ntasks = (1 << 7); ntasks <= MaxNTasks; ntasks *= 2) {
      const double normalized_par_to_seq_ratio = bench_sched_perf(ntasks);
      if (Verbosity) {
        std::cout << "TSZ=" << tile_sz << " NTASKS=" << ntasks << " r=";
      }
      std::cout << std::setw(8) << normalized_par_to_seq_ratio << ' ';
      if (Verbosity) {
        std::cout << std::endl;
      }
    }
    //std::cout << '%' << tile_sz;
    std::cout << std::endl;
  }
  
  return 0;
}

