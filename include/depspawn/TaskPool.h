///
/// \file     TaskPool.h
/// \brief    Pool of threads that execute tasks from a shared pool in FIFO
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#ifndef __TASKPOOL_H_
#define __TASKPOOL_H_

#include <atomic>
#include <cassert>
#include <boost/lockfree/queue.hpp>
#include "ThreadPool.h"
#include "LinkedListPool.h"

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
  boost::lockfree::queue<Task *, boost::lockfree::fixed_sized<true>> queue_;
  LinkedListPool<Task, false> task_pool_;
  volatile bool finish_;
  std::atomic<int> busy_threads_; ///< Becomes 0 only when all the pool threads run out of work

  void run(Task * const p)
  {
    p->run();
    p->~Task();
    task_pool_.free(p);
  }

  void main()
  {
    while (!finish_) {
      empty_queue();
      busy_threads_.fetch_sub(1);
      while(!finish_ && queue_.empty()) {}
      busy_threads_.fetch_add(1);
    }

    busy_threads_.fetch_sub(1);
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
  queue_{static_cast<size_t>(thread_pool_.nthreads() * avg_max_tasks_per_thread + !thread_pool_.nthreads())},
  task_pool_{thread_pool_.nthreads() * Default_Max_Tasks_Per_Thread + !thread_pool_.nthreads()},
  finish_{true},
  busy_threads_{0}
  {
    thread_pool_.setFunction(&TaskPool::main, this);

    if(launch) {
      launch_threads();
    }
  }

  TaskPool(const int nthreads, const bool launch) :
  TaskPool(nthreads, Default_Max_Tasks_Per_Thread, launch)
  { }

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

  /// \brief Tries to run one pending task, if available
  /// \return Whether any task was actually run
  bool try_run()
  { Task *p;

    const bool ret = queue_.pop(p);
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

  /// Works until there are no pending tasks, although they can be still in process
  void empty_queue()
  {
    while (try_run()) { }
  }

  /// Runs a parallel loop in the pool
  template<typename Index, typename F>
  void parallel_for(Index begin, Index end, Index step, const F& f, const bool relaunch_threads = true)
  {
    this->launch_threads();

    while (begin < end) {
      this->enqueue(f, begin);
      begin += step;
    }

    this->wait(relaunch_threads);
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
