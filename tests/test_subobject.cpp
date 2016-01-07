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
#include <cstdio>
#include <ctime>
#include <string>
#include <tbb/task_scheduler_init.h>
#include <tbb/tick_count.h>
#include <tbb/spin_mutex.h>  // This is only for sequencialicing parallel prints
#include "depspawn/depspawn.h"

using namespace depspawn;

struct t_struct {
  int a, b;
  
  std::string string() const {
    char tmp[128];
    sprintf(tmp, "(%d, %d)", a, b);
    return std::string(tmp);
  }
  
};

std::ostream &operator<<(std::ostream &os, const t_struct &a)
{
  os << a.string();
  return os;
}

tbb::spin_mutex  my_io_mutex; // This is only for sequencialicing parallel prints

#define LOG(...)   do{ tbb::spin_mutex::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)


tbb::tick_count t0;

t_struct s;


void mywait(float seconds)
{
  clock_t endwait;
  endwait = clock() + seconds * CLOCKS_PER_SEC ;
  while (clock() < endwait) {}
}

void f(int &i) {  
  LOG("f begin: " << (tbb::tick_count::now() - t0).seconds() << " with s=" << s);
  
  i++;
  
  mywait(2.f);
  
  i++;
  
  LOG("f finish: " << (tbb::tick_count::now() - t0).seconds() << " with s=" << s);
}

void g(t_struct& s) {  
  LOG("g begin: " << (tbb::tick_count::now() - t0).seconds() << " with s=" << s);
  
  s.a++;
  
  mywait(2.f);
  
  s.a++;
  
  LOG("g finish: " << (tbb::tick_count::now() - t0).seconds() << " with s=" << s);
}


int main()
{ 
  s.a = 0;
  s.b = 0;
  
  std::cout << CLOCKS_PER_SEC << std::endl;
  
  tbb::task_scheduler_init tbbinit;
  
  t0 = tbb::tick_count::now();
  
  spawn(f, s.a); //Sets s.a to 2
  
  spawn(f, s.b); //Sets s.b to 2

  spawn(g, s); //Sets s.a to 4
  
  spawn(f, s.a); //Sets s.a to 6
  
  spawn(f, s.b); //Sets s.b to 4
  
  wait_for_all();

  std::cout << "Final s.a =" << s.a << " s.b = " << s.b << std::endl;

  const bool test_ok = ((s.a == 6) && (s.b == 4));
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
