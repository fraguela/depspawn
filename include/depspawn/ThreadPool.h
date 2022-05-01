/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2022 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
*/

///
/// \file     ThreadPool.h
/// \brief    Provides a resizeable reusable pool of threads
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#ifndef __THREADPOOL_H_
#define __THREADPOOL_H_

#include <vector>
#include <thread>
#include <functional>
#include <stdexcept>
#include <mutex>
#include <condition_variable>

namespace depspawn {

namespace internal {

/// \brief Resizeable and reusable pool of threads
/// \internal It must be manipulated by an external main thread, not one in the pool
class ThreadPool {
  
  std::vector<std::thread> threads_;
  std::mutex mutex_;
  std::condition_variable cond_var_;
  std::function<void()> func_;  ///< Function simultaneously run by the active threads in the pool
  volatile int nthreads_in_use_; ///< Number of threads that participate in parallel executions
  volatile int count_;
  volatile bool ready_, finish_;

  void main()
  {
    while (!finish_) {
      std::unique_lock<std::mutex> my_lock(mutex_);
      while (!ready_ && !finish_) {
        cond_var_.wait(my_lock);
      }
      const int my_id = count_++;
      if(count_ == threads_.size()) {
        ready_ = false;
      }
      my_lock.unlock();

      if (!finish_ && (my_id < nthreads_in_use_)) {
        func_();
      }

      while(ready_ && !finish_) {} // ensure all threads restarted
      my_lock.lock();
      count_--;
      my_lock.unlock();

      //wait();
    }
  }

public:
  
  /// \brief Builds thread pool
  /// \param n Number of threads in the pool. Defaults to 0.
  ///          -x will build one thread per hardware thread - abs(x). See resize()
  ///
  /// Threads are inactive until launch_theads() is invoked
  ThreadPool(const int n = 0) :
  func_{[]{}}, count_{0}, ready_{false}, finish_{false}
  {
    resize(n);
  }
  
  int nthreads() const noexcept { return nthreads_in_use_; }

  /// \brief Change the number of threads in the pool
  /// \param new_nthreads New number of threads. If it is negative, the new number
  ///                     is hardware_concurrency()-abs(new_nthreads).
  /// \throws std::domain_error After adjustment the number of threads is still negative
  /// \internal When the new number is larger than the number of threads currently
  ///           in the pool, new threads are created. But when the new number is smaller
  ///           the class simply changes the number of threads in use so that only the
  ///           required ones do work
  void resize(int new_nthreads)
  {

    if(new_nthreads < 0 ) {
      new_nthreads += static_cast<int>(std::thread::hardware_concurrency());
      if (new_nthreads < 0) {
        throw std::domain_error("Negative number of threads");
      }
    }

    wait();

    // can only grow
    while (threads_.size() < static_cast<size_t>(new_nthreads)) {
      threads_.emplace_back(&ThreadPool::main, this);
    }
    nthreads_in_use_ = new_nthreads;
  }

  /// Launchs the active threads to run once the function provided
  void launch_theads()
  {
    if (nthreads_in_use_) {
      wait();
      {
        std::lock_guard<std::mutex> my_guard_lock(mutex_);
        ready_ = true;
      }
      cond_var_.notify_all();
    }
  }
  
  /// Define function to be run by the threads with arguments
  template<class F, class... Args>
  void set_function(F&& f, Args&&... args)
  {
    func_ = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
  }
  
  /// Spin wait until all threads finished.
  /// The threads remain unactive until a new launch_threads()
  void wait() const noexcept
  {
    while(ready_ || count_) {};
  }
  
  ~ThreadPool()
  {
    wait();
    
    finish_ = true;
    launch_theads();
    for (auto& thread : threads_) {
      thread.join();
    }
  }
  
};

} //namespace  internal

} //namespace depspawn

#endif
