
/// \file     spin_lock.h
/// \brief    Spin mutex/lock
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>

#ifndef __SPIN_MUTEX_H
#define __SPIN_MUTEX_H

#include <atomic>

/// Control based on lock/unlock on a std::atomic_flag
struct spin_mutex_t {
  
  std::atomic_flag lock_ = ATOMIC_FLAG_INIT;  ///< Controls access to the stored object
  
  spin_mutex_t() noexcept
  { }
  
  void lock() noexcept {
    while (lock_.test_and_set(std::memory_order_acquire));  // acquire lock
  }
  
  bool try_lock() noexcept {
    return !lock_.test_and_set(std::memory_order_acquire);
  }
  
  void unlock() noexcept {
    lock_.clear(std::memory_order_release);
  }
  
  /// Provides a scoped lock on the spin mutex based on RAII
  struct scoped_lock {

    spin_mutex_t& m_;

    scoped_lock(spin_mutex_t& m) : m_(m)
    { m_.lock(); }
    
    ~scoped_lock()
    { m_.unlock(); }
  };

};

#endif
