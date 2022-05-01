/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2022 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
*/

///
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#include <iostream>
#include "depspawn/depspawn.h"

using namespace depspawn;

struct t_struct {
  int a, b;
};

t_struct s;

void f(int &i)
{
  i++;
}

void g(t_struct& s)
{
  s.a++;
}

int main()
{ 
  s.a = 0;
  s.b = 0;
  
  std::cout << "Using " << get_num_threads() << " threads and a task queue limit of " << get_task_queue_limit() << '\n';
  
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
