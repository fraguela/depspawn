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

#include <iostream>
#include <ctime>
#include <tbb/tick_count.h>
#include <tbb/spin_mutex.h>  // This is only for sequencialicing parallel prints
#include "depspawn/depspawn.h"

using namespace depspawn;

tbb::spin_mutex  my_io_mutex; // This is only for sequencialicing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)

tbb::tick_count t0;

int x = 0;
int y = 0;

void mywait(float seconds)
{
  clock_t endwait;
  endwait = clock() + seconds * CLOCKS_PER_SEC ;
  while (clock() < endwait) {}
}

const char *name(const int &p)
{
  return (&p == &x) ? "x" : "y";
}

void display()
{
  LOG("Current x =" << x << " y = " << y);
}
/// Adds n units and requires s seconds
void add(int &i, int n, float s) {
  LOG("f  begin: " << (tbb::tick_count::now() - t0).seconds() << " with " << name(i) << '=' << i);
  
  mywait(s);

  i += n;

  LOG("f finish: " << (tbb::tick_count::now() - t0).seconds() << " with " << name(i) << '=' << i);
}

void subtasks_test(bool &test_ok)
{
  spawn(add, y, 2, 0.1f);
  spawn(add, y, 3, 0.1f);
  
  wait_for_subtasks();
  
  test_ok = test_ok && (y == 15);
  display();
  
  spawn(add, x, 2, 0.1f);
  
  // it only waits for the last task, but it depends on the
  //previous tasks on x, so it has to wait for all of them
  //and thus x==5, instead of just x==2.
  wait_for_subtasks();
  
  test_ok = test_ok && (x == 5);
  display();
}

int main()
{
  display();
  
  set_threads();
  
  t0 = tbb::tick_count::now();
  
  spawn(add, x, 2, 1.5f); //Sets x to 2
  spawn(add, x, 1, 1.5f); //Sets x to 3
  spawn(add, y, 3, 0.1f); //Sets y to 2
  spawn(add, y, 2, 0.1f); //Sets y to 5
  
  wait_for(y);
  bool test_ok = (y == 5);
  display();
  
  spawn(add, y, 5, 0.1f); //Sets y to 10
  
  wait_for(y);
  test_ok = test_ok && (y == 10);
  display();
  
  // tests wait_for_subtasks
  spawn(subtasks_test, test_ok);
  
  wait_for_all();
  test_ok = test_ok && (x == 5);
  display();

  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
