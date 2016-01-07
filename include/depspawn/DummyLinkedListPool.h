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
/// \file     DummyLinkedListPool.h
/// \brief    Provides an object with the same API as the LinkedListPool, but without any pooling
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#ifndef __DUMMYLINKEDLISTPOOL_H
#define __DUMMYLINKEDLISTPOOL_H

#include <cstdlib>
#include <tbb/scalable_allocator.h>

/// \brief Provides an API analogous to that of LinkedListPool but without providing an actual pool
/// \tparam T type of the objects of the fictional pool
/// \tparam SCALABLE Whether the heap is managed with std::malloc/free (false) or tbb::scalable_malloc/scalable_free (true)
template <typename T, bool SCALABLE>
class DummyLinkedListPool {

public:
  
  /// Constructor. It dimisses its arguments
  constexpr DummyLinkedListPool(int chunkSize = 1, int min_t_size = sizeof(T))
  { }

  /// Return an item to the pool (just deallocates it)
  static void free(T* const datain)
  {
    datain->~T();
    if(SCALABLE) scalable_free(datain); else std::free(datain);
  }
  
  /// Return a linked list of items to the pool (just deallocates them all)
  static void freeLinkedList(T* const datain, T* const last_datain)
  { T * const end = last_datain->next;
    
    T * p = datain;
    do {
      T * next = p->next;
      free(p);
      p = next;
    } while(p != end);
  }
  
  /// Return a linked list of items to the pool (i.e., deallocate), when each one needs to do some clean up
  template<typename F>
  static void freeLinkedList(T* const datain, F&& f)
  {
    T * p = datain;
    do {
      f(p);
      T * next = p->next;
      free(p);
      p = next;
    } while(p != nullptr);
  }
  
  /// Get an item from the pool (just allocate and default-construct it)
  static T* malloc()
  {
    T * ret =  static_cast<T *>(SCALABLE ? scalable_malloc(sizeof(T)) : std::malloc(sizeof(T)));
    new (ret) T();
    return ret;
  }

  /// Get an item from the pool (i.e., allocate), initializing it with a non- default constructor
  template<typename... Args>
  static T* malloc(Args&&... args)
  {
    T * ret =  static_cast<T *>(SCALABLE ? scalable_malloc(sizeof(T)) : std::malloc(sizeof(T)));
    new (ret) T(std::forward<Args>(args)...);
    return ret;
  }
  
  /* Not in use.
  T* malloc(int n)
  {
  }
  */
  
};

#endif // __DUMMYLINKEDLISTPOOL_H
