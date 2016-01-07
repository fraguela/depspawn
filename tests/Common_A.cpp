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
