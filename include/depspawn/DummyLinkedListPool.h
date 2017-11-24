/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2017 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
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
  
  /// Return a linked list of items to the pool when the end is known (just deallocates them all)
  static void freeLinkedList(T* const datain, T* const last_datain)
  { T * const end = static_cast<T *>(last_datain->next);
    
    T * p = datain;
    do {
      T * next = static_cast<T *>(p->next);
      free(p);
      p = next;
    } while(p != end);
  }
  
  /// Return a linked list of items to the pool when the end is unknown (just deallocates them all)
  void freeLinkedList(T* const datain)
  {
    T * p = datain;
    do {
      T * next = static_cast<T *>(p->next);
      free(p);
      p = next;
    } while(p != nullptr);
  }
  
  /// Return a linked list of items to the pool (i.e., deallocate), when each one needs to do some clean up
  template<typename F>
  static void freeLinkedList(T* const datain, F&& f)
  {
    T * p = datain;
    do {
      f(p);
      T * next = static_cast<T *>(p->next);
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
