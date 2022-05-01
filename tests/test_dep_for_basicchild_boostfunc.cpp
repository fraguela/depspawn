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
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <chrono>
#include "depspawn/depspawn.h"
#include "common_io.cpp"  // This is only for serializing parallel prints

using namespace depspawn;

//#define spawn(f,a...) f(a)

//TODO: Cuando F funcione, desmarcar el spawn de g

/////////////////////////////////////////////////////////

#include "common_A.cpp"

/////////////////////////////////////////////////////////

std::chrono::time_point<std::chrono::high_resolution_clock> t0;


//This function runs for long
void f(int &i) {  
  LOG("f begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  
  { // This is just used to make a delay
    A a(400);
  
    a.do_comp();
  }
  
  i = 10;
  
  LOG("f finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i);
}

void h(int &i, int s) {
  LOG("h begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i << " +=" << s);
  
  i += s;  
  
  LOG("h finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() << " with i=" << i);
} 

void g(int *ptr) {
  LOG("g begin: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
  
  boost::function<void(int&)> binded_h = boost::bind(h, _1, 10);
  
  //binded_h(*ptr); //Test to see that the function works
  
  spawn(binded_h, (*ptr));
  
  LOG("g finish: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count());
}


int main()
{ int i = 0; //i is in the stack

  t0 = std::chrono::high_resolution_clock::now();

  //spawn(f, Ignore<int>(i)); //With this version h does not wait for f
  
  spawn(f, i);      //f runs for long. Sets i=10
  
  spawn(g, (&i));   //g gets the argument as a pointer, so it should run in parallel as no dependency is detected
  
  wait_for_all();

  std::cout << "Final i=" << i << std::endl;
  
  const bool test_ok = (i == 20);
  
  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
  
  return !test_ok;
}
