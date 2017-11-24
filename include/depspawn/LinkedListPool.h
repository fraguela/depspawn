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
/// \file     LinkedListPool.h
/// \brief    Provides a simple pool based on a linked list with atomic operations
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#ifndef __LINKEDLISTPOOL_H
#define __LINKEDLISTPOOL_H

#include <cstdlib>
#include <vector>
#include <atomic>
#include <tbb/scalable_allocator.h>

/// \brief Provides a common API for heap allocation/deallocation
/// \tparam SCALABLE Whether the heap is managed with std::malloc/free (false) or tbb::scalable_malloc/scalable_free (true)
template <bool SCALABLE>
struct PoolAllocator_malloc_free
{
  typedef std::size_t size_type; //!< Unsigned integral type that can represent the size of the largest object to be allocated.
  typedef std::ptrdiff_t difference_type; //!< Signed integral type that can represent the difference of any two pointers.
  
  /// Allocate nbytes bytes of space in the head
  static char * malloc(const size_type nbytes)
  { return static_cast<char *>(SCALABLE ? scalable_malloc(nbytes) : std::malloc(nbytes)); }
  
  /// Deallocate the head space pointed by block
  static void free(char * const block)
  { if(SCALABLE) scalable_free(block); else std::free(block); }
  
};

/// \brief Pool implemented by means of a linked list with atomic operations
/// \tparam T type of the objects of the pool
/// \tparam SCALABLE Whether the heap is managed with std::malloc/free (false) or tbb::scalable_malloc/scalable_free (true)
template <typename T, bool SCALABLE>
class LinkedListPool {
  
  typedef std::vector<T *> vector_t;
  
  vector_t v_;            ///< Stores all the chunks allocated by this pool
  const int chunkSize_;   ///< How many holders to allocate each time
  const int minTSize_;    ///< Space allocated for each object
  std::atomic<T *> head_; ///< Current head of the pool
  std::atomic_flag pool_mutex_; ///< mutex for global critical sections in the pool (only in allocate)
  
  /// Allocate a new chunk of chunkSize_ elements for the pool
  void allocate() {
    char * baseptr = PoolAllocator_malloc_free<SCALABLE>::malloc(chunkSize_ * minTSize_);
    char * const endptr = baseptr +  minTSize_ * (chunkSize_ - 1);
    T * const h = reinterpret_cast<T *>(baseptr);
    T * const q = reinterpret_cast<T *>(endptr);
    T * p = h;
    do {
      baseptr += minTSize_;
      new (p) T();
      p->next = reinterpret_cast<T *>(baseptr); //invalid for p=q. Will be corrected during linking
      p = static_cast<T *>(p->next);
    } while (baseptr <= endptr);
    
    while (pool_mutex_.test_and_set(std::memory_order_acquire));
    
    v_.push_back(h);

    freeLinkedList(h, q);
    
    pool_mutex_.clear(std::memory_order_release);
  }

  /* Whether there are at leat \c n elements in the list that starts in \c p,
     returning in \c q the last element of such sub-list.
   
      Not in use but should be ok
   
   \pre p != nullptr
   \pre n >= 1
   
  bool length(T *p, int n, T* &q) const {
    
    do {
      q = p;
      p = p->next;
      --n;
    } while (n && *p);
    
    return !n;
  }
  */
  
public:
  
  /// \brief Constructor
  /// \param chunkSize  number of elements to allocate at once in chunk when the pool is empty
  /// \param min_t_size minimum space to allocate for each item.
  LinkedListPool(int chunkSize = 1, int min_t_size = sizeof(T)) :
  chunkSize_(chunkSize),
  minTSize_(sizeof(T) > min_t_size ? sizeof(T) : min_t_size),
  head_(nullptr),
  pool_mutex_(ATOMIC_FLAG_INIT)
  {
    //BBF: Test chunkSize > 0 ?
    allocate();
  }

  ~LinkedListPool()
  {
    typename vector_t::const_iterator const itend = v_.end();
    for(typename vector_t::const_iterator it = v_.begin(); it != itend; ++it)
      PoolAllocator_malloc_free<SCALABLE>::free(reinterpret_cast<char*>(*it));
  }

  /// Return an item to the pool
  void free(T* const datain) noexcept
  { T *p;
    
    p = head_;
    do {
      datain->next = p;
    } while(!head_.compare_exchange_weak(p, datain)); //while(head_.compare_and_swap(datain, p) != p);
  }
  
  /// Return a linked list of items to the pool when the end is known
  void freeLinkedList(T* const datain, T* const last_datain) noexcept
  { T *p;
    
    p = head_;
    do {
      last_datain->next = p;
    } while(!head_.compare_exchange_weak(p, datain)); //while(head_.compare_and_swap(datain, p) != p);
  }
  
  /// Return a linked list of items to the pool when the end is unknown
  void freeLinkedList(T* const datain) noexcept
  {
    T* p = datain;
    
    do {
      
      if (p->next == nullptr) {
        break;
      } else {
        p = static_cast<T *>(p->next);
      }
      
    } while (1);
    
    freeLinkedList(datain, p);
  }

  /// Return a linked list of items to the pool, when each one needs to do some clean up
  template<typename F>
  void freeLinkedList(T* const datain, F&& f)
  {
    T* p = datain;
    
    do {
      
      f(p);
      
      if (p->next == nullptr) {
        break;
      } else {
        p = static_cast<T *>(p->next);
      }
      
    } while (1);
    
    freeLinkedList(datain, p);
  }
  
  /// Get an item from the pool
  T* malloc()
  { T *ret, *next_val;
    
    ret = head_;
    do {
      while(ret == nullptr) {
        allocate();
        ret = head_;
      }
      next_val = static_cast<T *>(ret->next);
    } while(!head_.compare_exchange_weak(ret, next_val)); //while(head_.compare_and_swap(next_val, ret) != ret);
    
    //BBF: Notice that we do not make a new (ret) T()
    return ret;
  }

  /// Get an item from the pool, initializing it with a constructor
  template<typename... Args>
  T* malloc(Args&&... args)
  {
    T * const ret = this->malloc();
    new (ret) T(std::forward<Args>(args)...);
    return ret;
  }
  
  /* Not in use, but should be ok.
  T* malloc(int n)
  { T *ret, *q, *next_val;
   
    ret = head_;
    do {
      while((ret == nullptr) || !length(ret, n, q)) {
        allocate();
        ret = head_;
      }
      next_val = static_cast<T *>(q->next);
    } while(!head_.compare_exchange_weak(ret, next_val)); //while(head_.compare_and_swap(next_val, ret) != ret);
    
    q->next = nullptr;
    return ret;
  }
  */
  
};

#endif // __LINKEDLISTPOOL_H
