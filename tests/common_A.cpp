/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2022 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
*/

///
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

// Class used for the test on destruction
class A {
  volatile int n_;
  volatile float *a, *b, *c;
  
public:
  A(int n)
  : n_(n), a(new float[n *n ]), b(new float[n *n ]), c(new float[n *n ])
  {
    LOG("Building A of sz=" << n_);
    
    for(int i = 0; i < n_; i++)
      for(int j = 0; j < n_; j++) {
        c[i*n+j] = 0;
        a[i*n+j] = i + j;
        b[i*n+j] = i - j;
      }
    
  }
  
  int size() const { return n_; }
  
  volatile float& operator()(int i, int j)       { return c[i * n_ + j]; }
  
  float           operator()(int i, int j) const { return c[i * n_ + j]; }
  
  void do_comp();
  
  float res() { return c[0] + c[n_ * n_ - 1]; }
  
  ~A() {
    LOG("Destroying A of sz=" << n_);
    delete [] a;
    delete [] b;
    delete [] c;
  }
  
};

void A::do_comp() {
  for(int i = 0; i < n_; i++)
    for(int j = 0; j < n_; j++)
      for(int k = 0; k < n_; k++)
        c[i*n_+j] += a[i*n_+k] * b[k*n_+j];
}
