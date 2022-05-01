/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2022 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
*/

///
/// \file     DummyLinkedListPool.h
/// \brief    Provides an object with the same API as the LinkedListPool, but without any pooling
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#ifndef __DUMMYLINKEDLISTPOOL_H
#define __DUMMYLINKEDLISTPOOL_H

#include <cstdlib>

#ifdef DEPSPAWN_USE_TBB
#include <tbb/scalable_allocator.h>
#endif

/// \brief Provides an API analogous to that of LinkedListPool but without providing an actual pool
/// \tparam T type of the objects of the fictional pool
/// \tparam SCALABLE Whether the heap is managed with std::malloc/free (false) or tbb::scalable_malloc/scalable_free (true)
template <typename T, bool SCALABLE>
class DummyLinkedListPool {


  static T *malloc_api()
  {
    return static_cast<T *>(
#ifdef DEPSPAWN_USE_TBB
    SCALABLE ? scalable_malloc(sizeof(T)) : std::malloc(sizeof(T))
#else
                            std::malloc(sizeof(T))
#endif
                            );
  }


public:
  
  /// Constructor. It dimisses its arguments
  constexpr DummyLinkedListPool(int chunkSize = 1, int min_t_size = sizeof(T))
  { }

  /// Return an item to the pool (just deallocates it)
  static void free(T* const datain)
  {
    datain->~T();

#ifdef DEPSPAWN_USE_TBB
    if(SCALABLE)
      scalable_free(datain);
    else
#endif
      std::free(datain);
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
    T * ret = malloc_api();
    new (ret) T();
    return ret;
  }

  /// Get an item from the pool (i.e., allocate), initializing it with a non- default constructor
  template<typename... Args>
  static T* malloc(Args&&... args)
  {
    T * ret =  malloc_api();
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
