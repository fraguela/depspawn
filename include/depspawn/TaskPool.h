/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2021 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
*/

///
/// \file     TaskPool.h
/// \brief    Pool of threads that execute tasks from a shared pool in FIFO
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#ifndef __TASKPOOL_H_
#define __TASKPOOL_H_

#include <atomic>
#include <boost/lockfree/queue.hpp>
#include "depspawn/ThreadPool.h"
#include "depspawn/LinkedListPool.h"

namespace depspawn {

namespace internal {

struct Workitem;

class TaskPool {

  public:
  
  struct Task {

    std::function<void()> func_;
    Workitem *ctx_;
    Task *next; //< used by LinkedListPool and pointer to TaskPool
    
    Task() :
    ctx_{nullptr},
    next{nullptr}
    {}
    
    template<typename F>
    Task(TaskPool * const pt, Workitem *const ctx, const F& f) :
    func_{f},
    ctx_{ctx},
    next{reinterpret_cast<Task *>(pt)}
    {}

    template<typename F>
    Task(TaskPool * const pt, Workitem *const ctx, F&& f) :
    func_{std::move(f)},
    ctx_{ctx},
    next{reinterpret_cast<Task *>(pt)}
    {}

    void run(); //See depspawn.cpp

  };

  static constexpr int Default_Max_Tasks_Per_Thread = 4;

private:
  
  ThreadPool thread_pool_;
  const size_t queue_size_;
  boost::lockfree::queue<Task *, boost::lockfree::fixed_sized<true>> queue_;
  boost::lockfree::queue<Task *, boost::lockfree::fixed_sized<true>> hp_queue_;
  LinkedListPool<Task, false> task_pool_;
  volatile bool finish_;          ///< Whether wait for task pool to finish has been requested
  std::atomic<int> busy_threads_; ///< Becomes 0 only when all the pool threads run out of work
  std::function<void()> idle_func_; ///< Function to run when there are no tasks

  void free(Task * const p)
  {
    p->~Task();
    task_pool_.free(p);
  }

  void run(Task * const p)
  {
    p->run();
    free(p);
  }

  void main()
  {
    while (!finish_) {
      empty_queue();
      busy_threads_.fetch_sub(1);
      while(!finish_ && empty()) {
        if (idle_func_) {
          idle_func_();
        }
      }
      busy_threads_.fetch_add(1);
    }

    busy_threads_.fetch_sub(1);
  }

  template<typename Index, typename F>
  void common_parallel_for(Index begin, Index end, Index step, const F& f, const bool relaunch_threads, const bool is_hp)
  {
    this->launch_threads();

    while (begin < end) {
      if (is_hp) {
        this->hp_enqueue(f, begin);
      } else {
        this->enqueue(f, begin);
      }
      begin += step;
    }

    this->wait(relaunch_threads);
  }

  template<typename Index, typename F>
  void common_soft_parallel_for(Index begin, Index end, Index step, const F& f, const bool is_hp)
  { std::atomic<Index> n_tasks {((end - begin) + (step - 1)) / step};

    const auto my_f = [&f, &n_tasks](Index x) { f(x); n_tasks.fetch_sub(1); };

    while (begin < end) {
      if (is_hp) {
        this->hp_enqueue(my_f, begin);
      } else {
        this->enqueue(my_f, begin);
      }
      begin += step;
    }

    while (n_tasks.load()) {
      if (is_hp) {
        hp_try_run();
      } else {
        try_run();
      }
    }
  }

public:

  /// \brief Builds the task pool
  /// \param nthreads Number of threads in the pool.
  ///                 -x will build one thread per hardware thread - abs(x). See ThreadPool::resize()
  /// \param avg_max_tasks_per_thread Average number of tasks per thread supported before
  ///                                 the pushing thread has to run tasks from the pool.
  ///                                 That is, the task queue has nthreads*avg_max_tasks_per_thread
  ///                                 positions, and if full, the pushing thread has to run tasks
  ///                                 from the pool until it is able to push the new task.
  ///                                 Defaults to Default_Max_Tasks_Per_Thread
  /// \param launch Whether to launch inmediately the threads to execution. True by default
  TaskPool(const int nthreads, const int avg_max_tasks_per_thread = Default_Max_Tasks_Per_Thread, const bool launch = true) :
  thread_pool_{nthreads},
  queue_size_{static_cast<size_t>(thread_pool_.nthreads() * avg_max_tasks_per_thread + !thread_pool_.nthreads())},
  queue_{queue_size_},
  hp_queue_{queue_size_},
  task_pool_{thread_pool_.nthreads() * Default_Max_Tasks_Per_Thread + !thread_pool_.nthreads()},
  finish_{true},
  busy_threads_{0}
  {
    thread_pool_.set_function(&TaskPool::main, this);

    if(launch) {
      launch_threads();
    }
  }

  TaskPool(const int nthreads, const bool launch) :
  TaskPool(nthreads, Default_Max_Tasks_Per_Thread, launch)
  { }

  /// \brief Function to run while a thread is idle
  /// \internal Do not change while the pool is active
  template<typename F>
  void set_idle_function(const F& f) {
    assert(finish_);
    idle_func_ = f;
  }

  size_t queue_size() const noexcept { return queue_size_; }

  /// \brief Whether the task queue is empty. Tasks can be still under execution
  bool empty() const noexcept { return hp_queue_.empty() && queue_.empty(); }

  /// Return whether the pool threads are currently running
  bool is_running() const noexcept { return !finish_; }

  /// Launchs the threads to execution if they are not running
  void launch_threads()
  {
    if(finish_) {
      busy_threads_.store(thread_pool_.nthreads());
      finish_ = false;
      thread_pool_.launch_theads();
    }
  }

  /// Return the number of threads in the pool
  int nthreads() const noexcept { return thread_pool_.nthreads(); }

  /// \brief Returns the number of busy threads in the pool
  ///
  /// It is not exactly the number of threads making useful work because they can also be contending for work
  int nbusy_threads() const noexcept { return busy_threads_.load(std::memory_order_relaxed); }

  /// \brief Tries to run one pending task, if available
  /// \return Whether any task was actually run
  bool try_run()
  { Task *p;

    const bool ret = hp_queue_.pop(p) || queue_.pop(p);
    if (ret) {
      run(p);
    }

    return ret;
  }

  /// \brief Tries to run one HP pending task, if available
  /// \return Whether any task was actually run
  bool hp_try_run()
  { Task *p;
    
    const bool ret = hp_queue_.pop(p);
    if (ret) {
      run(p);
    }
    
    return ret;
  }

  /// Builds a task for running a function with a series of arguments
  template<class F, class... Args>
  Task *build_task(Workitem *ctx, F&& f, Args&&... args)
  {
    return task_pool_.malloc(this, ctx, std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  }

  /// Enqueues a task for running a function with a series of arguments
  template<class F, class... Args>
  void enqueue(F&& f, Args&&... args)
  {
    enqueue(build_task(nullptr, std::forward<F>(f), std::forward<Args>(args)...));
  }

  /// Enqueues a task for execution regardless of its number of dependencies
  void enqueue(Task * const task_ptr)
  {
    //automatically launching threads within enqueue is dangerous for unstructured/external tasks
    //launch_theads();

    while (!queue_.bounded_push(task_ptr)) {
      try_run();
    }
  }

  /// Enqueues a HP task for running a function with a series of arguments
  template<class F, class... Args>
  void hp_enqueue(F&& f, Args&&... args)
  {
    hp_enqueue(build_task(nullptr, std::forward<F>(f), std::forward<Args>(args)...));
  }

  /// Enqueues a HP task for execution regardless of its number of dependencies
  void hp_enqueue(Task * const task_ptr)
  {
    //automatically launching threads within enqueue is dangerous for unstructured/external tasks
    //launch_theads();

    while (!hp_queue_.bounded_push(task_ptr)) {
      hp_try_run();
    }
  }

  /// Tries to enqueue a task for running a function with a series of arguments
  /// and returns whether the enqueue was successfull
  template<class F, class... Args>
  bool try_enqueue(F&& f, Args&&... args)
  {
    Task * const ptask = build_task(nullptr, std::forward<F>(f), std::forward<Args>(args)...);
    const bool ret = try_enqueue(ptask);
    if (!ret) {
      free(ptask);
    }
    return ret;
  }

  /// Tries to enqueue a task and returns whether the enqueue was successfull
  /// @internal If unsuccessful the user is responsible for managing the task
  bool try_enqueue(Task * const task_ptr)
  {
    return queue_.bounded_push(task_ptr);
  }

  /// Works until there are no pending tasks, although they can be still in process
  void empty_queue()
  {
    while (try_run()) { }
  }

  /// Runs a parallel loop in the pool and waits for every task to finish.
  /// Should only be used at top level
  template<typename Index, typename F>
  void parallel_for(Index begin, Index end, Index step, const F& f, const bool relaunch_threads = true)
  {
    this->common_parallel_for(begin, end, step, f, relaunch_threads, false);
  }

  /// Runs a parallel loop in the pool and waits for its top-level children to finish execution
  template<typename Index, typename F>
  void soft_parallel_for(Index begin, Index end, Index step, const F& f)
  {
    this->common_soft_parallel_for(begin, end, step, f, false);
  }

  /// Runs a HP parallel loop in the pool and waits for every task,
  /// including non-HP tasks, to finish.
  /// Should only be used at top level
  template<typename Index, typename F>
  void hp_parallel_for(Index begin, Index end, Index step, const F& f, const bool relaunch_threads = true)
  {
    this->common_parallel_for(begin, end, step, f, relaunch_threads, true);
  }

  /// Runs a HP parallel loop in the pool and waits for its top-level children to finish execution
  template<typename Index, typename F>
  void hp_soft_parallel_for(Index begin, Index end, Index step, const F& f)
  {
    this->common_soft_parallel_for(begin, end, step, f, true);
  }

  /// \brief Active wait until all tasks complete
  /// \param relaunch_threads if true, the threads are relaunched after the wait.
  ///                         Otherwise, they sleep until launch_theads() is invoked
  void wait(const bool relaunch_threads = true)
  {
    do {
      empty_queue();
    } while(busy_threads_.load(std::memory_order_relaxed));

    finish_ = true;
    thread_pool_.wait();

    if (relaunch_threads) {
      launch_threads();
    }
  }

  ~TaskPool()
  {
    wait(false);
  }

};

} //namespace  internal

} //namespace depspawn

#endif
