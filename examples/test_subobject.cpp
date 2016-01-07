/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2015 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
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

#include "depspawn/depspawn.h"

using namespace depspawn;

struct t_struct {
  int a, b;
};

t_struct s;

void f(int &i) {
  i++;
}

void g(t_struct& s) {  
  
  s.a++;
}

int main()
{ 
  s.a = 0;
  s.b = 0;
  
  set_threads(tbb::task_scheduler_init::automatic);
  
  spawn(f, s.a); //Sets s.a to 1
  
  spawn(f, s.b); //Sets s.b to 1

  spawn(g, s);   //Sets s.a to 2
  
  spawn(f, s.a); //Sets s.a to 3
  
  spawn(f, s.b); //Sets s.b to 2
  
  wait_for_all();

  std::cout << "Final s.a =" << s.a << " s.b = " << s.b << std::endl;

  const bool test_ok = ((s.a == 3) && (s.b == 2));
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
