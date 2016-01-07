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
#include <algorithm>
#include "depspawn/depspawn.h"

using namespace depspawn;


void fun() {
  volatile double a = 2.1, b = 3.2, c=-42.1;

  for(int i = 0; i < 1000; i++)
    for(int j = 0; j < 1000; j++)
      for(int k = 0; k < 1000; k++)
        a = a*b/c;
}

void f1(int&& a) { std::cout << "f1" << a << std::endl; fun(); }

void f2(int& b) { std::cout << "f2\n"; fun(); }

int main() {
  int p1, p2, p3;
  int p4, p5, p6;
  int p7, p8, p9;
  
  spawn(f1, p1);
  
  spawn(f2, p1);
  
  wait_for_all();

  return 0;
}

