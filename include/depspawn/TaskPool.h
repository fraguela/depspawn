///
/// \file     TaskPool.h
/// \brief    Pool of threads that execute tasks from a shared pool in FIFO
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///


#ifndef __TASKPOOL_H_
#define __TASKPOOL_H_

#include <boost/lockfree/queue.hpp>
#include <sstream>
#include "ThreadPool.h"
#include "LinkedListPool.h"


class TaskPool {

  public:
  
  struct Task {
    std::function<void()> func_;
    std::atomic<int> ndeps_;
    Task *next; //< used by LinkedListPool and pointer to TaskPool
    
    Task() :
    ndeps_{0},
    next{nullptr}
    {}
    
    template<typename F>
    Task(TaskPool * const pt, const F& f) :
    func_{f},
    ndeps_{0},
    next{reinterpret_cast<Task *>(pt)}
    {}

    template<typename F>
    Task(TaskPool * const pt, F&& f) :
    func_{std::move(f)},
    ndeps_{0},
    next{reinterpret_cast<Task *>(pt)}
    {}
    
    void ndeps(int i) noexcept { ndeps_.store(i, std::memory_order_relaxed); }

    int ndeps() const noexcept { return ndeps_.load(std::memory_order_relaxed); }

    void incr_deps(const int n = 1) noexcept { ndeps_.fetch_add(n); }

    void decr_deps(const int n = 1)
    {
      if (ndeps_.fetch_sub(n) == 1) {
        reinterpret_cast<TaskPool *>(next)->enqueue(this);
      }
    }

    /// \brief Waits for this specific task
    /// \internal Must be called before it is triggered by decr_deps, thus put 1 extra dep
    void wait()
    {
      reinterpret_cast<TaskPool *>(next)->wait_on(this);
    }

  };

  static constexpr int Default_Max_Tasks_Per_Thread = 4;

private:
  
  ThreadPool thread_pool_;
  boost::lockfree::queue<Task *, boost::lockfree::fixed_sized<true>> queue_;
  LinkedListPool<Task, false> task_pool_;
  volatile bool finish_;
  
  void run(Task *const p)
  {
    p->func_();
    p->~Task();
    task_pool_.free(p);
  }

  bool try_run()
  { Task *p;
    
    const bool ret = queue_.pop(p);
    if (ret) {
      run(p);
    }
    
    return ret;
  }

  void main()
  {
    while (!finish_) {
      empty_queue();
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
  queue_{static_cast<size_t>(thread_pool_.nthreads() * avg_max_tasks_per_thread)},
  task_pool_{thread_pool_.nthreads() * Default_Max_Tasks_Per_Thread},
  finish_{true}
  {
    thread_pool_.setFunction(&TaskPool::main, this);

    if(launch) {
      launch_threads();
    }
  }

  TaskPool(const int nthreads, const bool launch) :
  TaskPool(nthreads, Default_Max_Tasks_Per_Thread, launch)
  { }

  /// Launchs the threads to execution if they are not running
  void launch_threads()
  {
    if(finish_) {
      finish_ = false;
      thread_pool_.launch_theads();
    }
  }

  int nthreads() const noexcept { return thread_pool_.nthreads(); }
  
  template<class F, class... Args>
  Task *build_task(F&& f, Args&&... args)
  {
    return task_pool_.malloc(this, std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  }

  template<class F, class... Args>
  void enqueue(F&& f, Args&&... args)
  {
    enqueue(build_task(std::forward<F>(f), std::forward<Args>(args)...));
  }

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

  /// \brief Waits for a task to be ready and then runs it locally
  ///
  /// The task must have been allocated with one extra dependency
  void wait_on(Task * const task_ptr)
  {
    while (task_ptr->ndeps() != 1) {
      try_run();
    }
    run(task_ptr);
  }

  /// \brief Active wait until all tasks complete
  /// \param relaunch_threads if true, the threads are relaunched after the wait.
  ///                         Otherwise, they sleep until launch_theads() is invoked
  void wait(const bool relaunch_threads = true)
  {
    empty_queue();
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

#endif
