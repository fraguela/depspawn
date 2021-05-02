/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2021 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
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

#ifdef DEPSPAWN_USE_TBB
#include <tbb/scalable_allocator.h>
#endif

/// \brief Provides a common API for heap allocation/deallocation
/// \tparam SCALABLE Whether the heap is managed with std::malloc/free (false) or tbb::scalable_malloc/scalable_free (true)
template <bool SCALABLE>
struct PoolAllocator_malloc_free
{
  typedef std::size_t size_type; //!< Unsigned integral type that can represent the size of the largest object to be allocated.
  typedef std::ptrdiff_t difference_type; //!< Signed integral type that can represent the difference of any two pointers.
  
  /// Allocate nbytes bytes of space in the head
  static char * malloc(const size_type nbytes)
  { void *ret;

#ifdef DEPSPAWN_USE_TBB
    if(SCALABLE)
      ret = scalable_malloc(nbytes);
    else
#endif
      ret = std::malloc(nbytes);
    return static_cast<char *>(ret);
  }
  
  /// Deallocate the head space pointed by block
  static void free(char * const block)
  {
#ifdef DEPSPAWN_USE_TBB
    if(SCALABLE)
      scalable_free(block);
    else
#endif
      std::free(block);
  }
  
};

/// \brief Pool implemented by means of a linked list with atomic operations
/// \tparam T          type of the objects of the pool. They must have a field <tt>T * next</tt>
/// \tparam ALLOC_ONCE if true will only build the objects when they are first created and it will
///                    never invoke a destructor on them. If false, contruction/destruction
///                    happens upon request from/return to the pool.
/// \tparam SCALABLE Whether the heap is managed with std::malloc/free (false) or tbb::scalable_malloc/scalable_free (true)
template <typename T, bool ALLOC_ONCE, bool SCALABLE>
class LinkedListPool {

  static constexpr intptr_t H_MASK = 0xFFFF000000000000ULL;
  static constexpr intptr_t L_MASK = 0x0000FFFFFFFFFFFFULL;
  static constexpr intptr_t H_INCR = 0x0001000000000000ULL;

  /// Remove index from ptr
  static constexpr T* clean_ptr(T* const p) noexcept {
    return (T*) ((intptr_t)p & L_MASK);
  }

  /// Build new ptr with index
  /// \internal Assumes high 16 bits of new_node are 0. Otherwise call clean_ptr on it before.
  static constexpr T* next_ptr(T* const new_node, T* const cur_head) noexcept {
    return (T*) ((intptr_t)new_node | (((intptr_t)cur_head + H_INCR) & H_MASK));
  }

  typedef std::vector<T *> vector_t;
  
  vector_t v_;            ///< Stores all the chunks allocated by this pool
  const int chunkSize_;   ///< How many holders to allocate each time
  const int minTSize_;    ///< Space allocated for each object
  std::atomic<T *> head_; ///< Current head of the pool
  std::atomic_flag pool_mutex_ = ATOMIC_FLAG_INIT; ///< mutex for global critical sections in the pool (only in allocate)
  
  /// Allocate a new chunk of chunkSize_ elements for the pool
  void allocate() {
    char * baseptr = PoolAllocator_malloc_free<SCALABLE>::malloc(chunkSize_ * minTSize_);
    char * const endptr = baseptr +  minTSize_ * (chunkSize_ - 1);
    T * const h = reinterpret_cast<T *>(baseptr);
    T * const q = reinterpret_cast<T *>(endptr);
    T * p = h;
    do {
      baseptr += minTSize_;
      if (ALLOC_ONCE) {
        new (p) T();
      }
      p->next = reinterpret_cast<T *>(baseptr); //invalid for p=q. Will be corrected during linking
      p = static_cast<T *>(p->next);
    } while (baseptr <= endptr);
    
    while (pool_mutex_.test_and_set(std::memory_order_acquire));
    
    v_.push_back(h);

    freeLinkedList(h, q, true);
    
    pool_mutex_.clear(std::memory_order_release);
  }

  /// \brief Get an item from the pool without invoking a constructor
  T* intl_malloc()
  {
    T* ret = head_.load(std::memory_order_relaxed);
    do {
      while(clean_ptr(ret) == nullptr) {
        allocate();
        ret = head_.load(std::memory_order_relaxed);
      }
    } while(!head_.compare_exchange_weak(ret, next_ptr(clean_ptr(static_cast<T *>(clean_ptr(ret)->next)), ret)));
    
    //Notice that we do not make a new (ret) T()
    return clean_ptr(ret);
  }

public:
  
  /// \brief Constructor
  /// \param chunkSize  number of elements to allocate at once in chunk when the pool is empty
  /// \param min_t_size minimum space to allocate for each item.
  LinkedListPool(const int chunkSize = 1, const size_t min_t_size = sizeof(T)) :
  chunkSize_{(chunkSize < 1) ? 1 : chunkSize},
  minTSize_{static_cast<int>((sizeof(T) > min_t_size) ? sizeof(T) : min_t_size)},
  head_{nullptr}
  {
    allocate();
  }

  /// \brief Destructor
  /// \internal If ALLOC_ONCE is false, object destructors are not needed.
  ///           If it is true, we currenly assume the destructor is not necessary
  ~LinkedListPool()
  {
    typename vector_t::const_iterator const itend = v_.end();
    for(typename vector_t::const_iterator it = v_.begin(); it != itend; ++it)
      PoolAllocator_malloc_free<SCALABLE>::free(reinterpret_cast<char*>(*it));
  }

  /// Return an item to the pool
  void free(T* const datain) noexcept(ALLOC_ONCE)
  {
    if (!ALLOC_ONCE) {
      datain->~T();
    }

    datain->next = head_.load(std::memory_order_relaxed);
    while(!head_.compare_exchange_weak(datain->next, next_ptr(datain, datain->next)));
  }
  
  /// Return a linked list of items to the pool when the end is known
  void freeLinkedList(T* const datain, T* const last_datain, const bool no_dealloc = false) noexcept(ALLOC_ONCE)
  {
    if (!ALLOC_ONCE && !no_dealloc) {
      last_datain->next = nullptr;
      for (T* p = datain; p != nullptr; p = static_cast<T *>(p->next)) {
        p->~T();
      }
    }

    last_datain->next = head_.load(std::memory_order_relaxed);
    while(!head_.compare_exchange_weak(reinterpret_cast<T*&>(last_datain->next), next_ptr(datain, static_cast<T *>(last_datain->next))));
  }
  
  /// Return a linked list of items to the pool when the end is unknown
  void freeLinkedList(T* const datain) noexcept
  {
    T* p = datain;
    
    do {

      if (!ALLOC_ONCE) {
        p->~T();
      }

      if (p->next == nullptr) {
        break;
      } else {
        p = static_cast<T *>(p->next);
      }
      
    } while (1);
    
    freeLinkedList(datain, p, true);
  }

  /// Return a linked list of items to the pool, when each one needs to do some clean up
  template<typename F>
  void freeLinkedList(T* const datain, F&& f)
  {
    T* p = datain;
    
    do {

      f(p);

      if (!ALLOC_ONCE) {
        p->~T();
      }

      if (p->next == nullptr) {
        break;
      } else {
        p = static_cast<T *>(p->next);
      }
      
    } while (1);
    
    freeLinkedList(datain, p, true);
  }
  
  /// Get an item from the pool and built it with the default constructor, if neccessary
  T* malloc()
   {
     T * const ret = this->intl_malloc();
     if (!ALLOC_ONCE) {
       new (ret) T();
     }
     return ret;
   }

   /// Get an item from the pool and built it with a custom constructor
   template<typename... Args>
   T* malloc(Args&&... args)
   {
     T * const ret = this->intl_malloc();
     new (ret) T(std::forward<Args>(args)...);
     return ret;
   }

  /*
  void print_content() const
  {
    fprintf(stderr, "Pool content:\n");
    T* pf = head_.load(std::memory_order_relaxed);
    for(T* p = clean_ptr(pf);
        p != nullptr;
        pf = p->next, p = clean_ptr(pf)) {
      fprintf(stderr, "%4lx | %p\n", (size_t)pf >> 48, p);
    }
    fprintf(stderr, "------\n");
  }

  bool contains(T* const q) const
  {
    for(T* p = clean_ptr(head_.load(std::memory_order_relaxed));
        p != nullptr;
        p = clean_ptr(p->next)) {
      if (p == q) {
        return true;
      }
    }
    return false;
  }
  */

};

#endif // __LINKEDLISTPOOL_H
