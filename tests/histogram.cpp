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
