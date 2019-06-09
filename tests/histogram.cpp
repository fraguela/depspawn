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
#include <random>
#include <array>
#include <algorithm>
#include "depspawn/depspawn.h"

using namespace depspawn;

static const int N = 100000, RANGE = 100;

std::array<int, N> v;
std::array<int, RANGE> hist_par, hist_seq;

void incr(int &i) {
  i++;
}

int main()
{
  // Make some random numbers in the interval 0 -- RANGE -1
  std::random_device rd;
  std::uniform_int_distribution<int> dist(0, RANGE - 1);
  
  for (int i = 0; i < N; i++) {
    v[i] = dist(rd);
  }
  
  std::cout << "Data built" << std::endl;
  
  // Compute the histogram sequentially
  hist_seq.fill(0);
  for (int i = 0; i < N; i++)
    hist_seq[v[i]]++;

  std::cout << "Seq histogram built" << std::endl;

  // Compute the histogram in parallel
  hist_par.fill(0);
  
  set_threads();

  for (int i = 0; i < N; i++) {
    spawn(incr, hist_par[v[i]]);
  }
  
  wait_for_all();

  // Do they match?

  bool test_ok = std::equal(hist_seq.begin(), hist_seq.end(), hist_par.begin());
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
