/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2023 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
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

