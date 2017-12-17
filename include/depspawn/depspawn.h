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
/// \file     depspawn.h
/// \brief    Main header file, and only one that needs to be included in the client code
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#ifndef __DEPSPAWN_H
#define __DEPSPAWN_H

#include <functional>
#include <type_traits>
#include <utility>
#include <boost/function_types/function_arity.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/function.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/deref.hpp>
#include <tbb/task.h>
#include <atomic>
#include <tbb/task_scheduler_init.h>

#include "depspawn/fixed_defines.h"

#ifndef DEPSPAWN_SCALABLE_POOL
/// Whether the library pools will use std::malloc/free (false) or tbb::scalable_malloc/scalable_free (true)
#define DEPSPAWN_SCALABLE_POOL false
#endif

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef DEPSPAWN_DUMMY_POOL
#include "depspawn/DummyLinkedListPool.h"
#else
#include "depspawn/LinkedListPool.h"
#endif

#ifdef BZ_ARRAY_H
#ifndef DEPSPAWN_BLITZ
#error Only the Blitz++ version released with DepSpawn can be used with DepSpawn
#endif
#endif

#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#define DEPSPAWN_THREADLOCAL __thread
#elif defined(_MSC_VER)
#define DEPSPAWN_THREADLOCAL __declspec(thread)
#else
#define DEPSPAWN_THREADLOCAL thread_local
#endif

/// \namespace depspawn
/// \brief Public library API
namespace depspawn {
  
  
  
  /// Class for ignoring arguments when doing dependency analysis
  template<typename T>
  class Ignore {
  
    T t_;
  
  public:
    
    /// Constructor
    Ignore(T& t) :
    t_(t)
    {}
    
    /// Assignment
    Ignore<T> operator= (const T& other) {
      t_ = other;
      return *this;
    }
    
    /// Conversion to T
    operator T&() {
      return t_;
    }
    
    /* Formerly used in ref<Ignore<T>&&>::make(Ignore<T>& t)
       Now replaced by the conversion operator
    T& operator*() {
      return t_;
    }
    
    const T& operator*() const {
      return t_;
    }
    */
    
    /// For debugging
    void *addr() const { return (void *)&t_; }
  };
  
  /// Traits for arguments or parameters that should be ignored
  template<typename T>
  inline Ignore<T> ignore(T&& t) {
    return Ignore<T>(std::forward<T>(t));
  }
  

  /// \namespace depspawn::internal
  /// \brief Internal portions of the DepSpawn library
  namespace internal {
    
    /// \typedef Pool_t
    /// \brief Type for the pools used inside the library
#ifdef DEPSPAWN_DUMMY_POOL
    template<typename T, bool SCALABLE>
    using Pool_t = DummyLinkedListPool<T, SCALABLE>;
#else
    template<typename T, bool SCALABLE>
    using Pool_t = LinkedListPool<T, SCALABLE>;
#endif
    
    /** \name Lambda function to std::function translator
     *
     *  Workaround to be able to use lambda functions. It builds a std::function that encapsulates a lambda function
     */
    ///@{
    
    template<typename MembFuncPtr>
    struct memb_func_traits
    {};
    
    template< typename ClassT, typename R, typename... Arg>
    struct memb_func_traits<R ClassT::* (Arg...)>
    {
      typedef ClassT class_type;
      typedef R signature (Arg...);
    };
    
    template< typename ClassT, typename R, typename... Arg>
    struct memb_func_traits<R (ClassT::*) (Arg...) const>
    {
      typedef ClassT class_type;
      typedef R signature (Arg...);
    };
    
    template<typename F>
    struct function_type {
      typedef typename memb_func_traits<decltype(&F::operator())>::signature signature;
      typedef std::function<signature> type;
    };
    
    /* For pointer to member function spawn in Apple clang. 
       We choose ClassT& for the std::function 1st arg */
    template<typename ClassT, typename R, typename... Arg>
    struct function_type<R (ClassT::*) (Arg...)> {
      typedef R signature (ClassT&, Arg...);
      typedef std::function<signature> type;
    };

    /* For pointer to member function spawn in Apple clang.
       We choose ClassT& for the std::function 1st arg */
    template<typename ClassT, typename R, typename... Arg>
    struct function_type<R (ClassT::*) (Arg...) const> {
      typedef R signature (const ClassT&, Arg...);
      typedef std::function<signature> type;
    };
    
    template<typename F>
    typename function_type<F>::type make_function(F && f)
    {
      typedef typename function_type<F>::type result_type;
      return result_type(std::forward<F>(f));
    }
    ///@}
    
    /// \name Detects std::function and boost:function objects
    ///@{
    template<typename T>
    struct is_function_object :public boost::false_type {};
    
    template<typename T>
    struct is_function_object<std::function<T>> : public boost::true_type {};
    
    template<typename T>
    struct is_function_object<boost::function<T>> : public boost::true_type {};
    ///@}
    
    /// Data type for the base address of task arguments
    typedef size_t value_t;
    
    /// Range [first, second] of a Blitz++ array in a given dimension
    typedef std::pair<int, int> array_range_t;
    
    /// Trait used to detect Blitz++ arrays
    template<typename T>
    struct is_blitz_array : public boost::false_type {};
    
    /// Information for one argument in a spawn invocation
    struct arg_info {
      value_t addr;                ///< base address
      size_t size;                 ///< size
      array_range_t* array_range;  ///< subregion in each dimension (only used if it is an array)
      arg_info* next;              ///< next arg_info (sorted by addr)
      bool wr;                     ///< writable?
      char rank;                   ///< dimensions (if it is an array), 0 otherwise

      /// inserts this object in the list of args in order according to addr
      void insert_in_arglist(arg_info*& args);
      
      /// solves the overlap of non-array arg_infos during the insertion in the list of arguments of a workitem
      void solve_overlap(arg_info *other);
      
      /// Returns whether this arg_info overlaps with the preceeding arg_info other,
      /// or any of its following arg_info's on the same array, both defined on the same array
      bool overlap_array(const arg_info *other) const;
      
      /// Returns whether this arg_info is contained within the preceeding arg_info other,
      /// or any of its following arg_info's on the same array, both defined on the same array
      bool is_contained_array(const arg_info *other) const;

      /// Whether this object refers to an array selection
      bool is_array() const { return rank != 0; }

#ifdef DEPSPAWN_BLITZ
      /// Fills ins the information for actual Blitz++ arrays
      template<typename T>
      inline typename std::enable_if< is_blitz_array<T>::value >::type fill_in_array(const T& a) {
        addr = (value_t)a.block();
        const auto& storage = a.storage();
        rank = storage.originalDims();
        array_range = new array_range_t[rank];
        const blitz::TinyVector<int, 2> * const extent = storage.extent();
        for(int i = 0; i < rank; i++) {
          array_range[i] = std::make_pair(extent[i][0], extent[i][1]);
        }
      }
#endif

      /// Fills ins the information for non-array objects
      template<typename T>
      inline typename std::enable_if< ! is_blitz_array<T>::value >::type fill_in_array(const T& a) {
        addr        = (value_t)&a;
        rank        = 0;
        array_range = nullptr;
      }

      /// Class pool
      static Pool_t<arg_info, DEPSPAWN_SCALABLE_POOL> Pool;

    };
    
    /** \name Translation of actual arguments of spawn into types for binding
     */
    ///@{
    
    template<typename T>
    struct ref {
      static inline auto make(T& t) -> decltype(std::ref(t)) {
	return std::ref(t);
      }
    };
    
    template<typename T>
    struct ref<T&&> {
      static inline T& make(T& t) {
	return t;
      }
    };
    
    /// Transform the reference to Ignore into a reference to T
    template<typename T>
    struct ref<Ignore<T>&&> {
      static inline auto make(Ignore<T>& t) -> decltype(std::ref((T&)t)) {
	return std::ref((T&)t);
      }
    };

#ifdef DEPSPAWN_BLITZ
    template<typename type, int dim>
    struct ref<blitz::Array<type, dim>&&> {
      static inline blitz::Array<type, dim> make(blitz::Array<type, dim>& __t) {
	return __t;
      }
    };
    
    template<typename type, int dim>
    struct ref<blitz::Array<type, dim>&> {
      static inline blitz::Array<type, dim> make(blitz::Array<type, dim>& __t) {
	return __t;
      }
    };
    
    template<typename type, int dim>
    struct ref<const blitz::Array<type, dim>&> {
      static inline const blitz::Array<type, dim> make(const blitz::Array<type, dim>& __t) {
	return __t;
      }
    };
    
    template<typename type, int dim>
    struct ref<const blitz::Array<type, dim>> {
      static inline const blitz::Array<type, dim> make(const blitz::Array<type, dim>& __t) {
	return __t;
      }
    };
#endif // DEPSPAWN_BLITZ
    
    ///@}
    
    //////////////////////////////////
    
    template<typename T>
    struct is_ignored {
      static const bool value = false;
    };
    
    template<typename T>
    struct is_ignored<Ignore<T>&&> {
      static const bool value = true;
    };
    
    /////////////////////////////////////////

    /// Root task, from which all the spawned ones descend
    extern tbb::task* volatile master_task;
    
    /// Whether tasks are normally spawned (false) or enqueued (true)
    extern bool EnqueueTasks;

    struct AbstractBoxedFunction;
    struct AbstractRunner;
    
  } //namespace  internal

} //namespace depspawn


#include "depspawn/workitem.h"

namespace depspawn {
  
  namespace internal {
  
    /// Pointer to the father of the task currently being executed, or nullptr if it is the root
    extern DEPSPAWN_THREADLOCAL Workitem * enum_thr_spec_father;

    /// Initializes the DepSpawn-specific global variables for a parallel session
    extern void start_master();
    
    /// Common steps for wait_for operations
    extern void common_wait_for(arg_info *pargs, int nargs);
    
    /// Common elements to all BoxedFunction objects. Provides type-independent run() API
    struct AbstractBoxedFunction {
      
      Workitem* ctx_; ///< Workitem of the stolen work
      
      /// Constructor
      /// \param ctx Workitem whose work is being stolen
      AbstractBoxedFunction(Workitem* ctx)
      : ctx_(ctx)
      {}
      
      void run_in_env(bool from_wait);
  
      virtual ~AbstractBoxedFunction() {}
      
    protected:
      
      /// Runs the stolen function
      virtual void run() = 0;
    };
    
    /// Function stolen to a task
    template<typename Function>
    struct BoxedFunction : public AbstractBoxedFunction {
      
      Function f_; ///< stolen function
      
      /// Constructor
      /// \param ctx Workitem whose work is being stolen
      /// \param f   function that encapsulates the stolen work
      BoxedFunction(Workitem* ctx, Function&& f)
      : AbstractBoxedFunction(ctx), f_(std::move(f))
      {}
    
      virtual ~BoxedFunction() {}
      
    protected:
      
      /// Runs the stolen function
      void run() final {
        f_();
      }
    
    };
    
    /// Provides common function-independent elements for runner tasks
    struct AbstractRunner : public tbb::task {

      std::atomic<Workitem *> ctx_; ///< Workitem associated to this runner task
      
      /// Constructor
      /// \param ctx  Workitem associated to this runner task
      AbstractRunner(Workitem * ctx)
      : ctx_(ctx)
      { }

      /// Steal the work of this runner task
      virtual AbstractBoxedFunction * steal(Workitem *ctx) = 0;
      
      virtual ~AbstractRunner() {}
    };
    
    /// Manages after-run ops
    template<typename Function>
    struct runner : public AbstractRunner {

      Function f_; ///< Work to execute
      
      /// Constructor
      /// \param ctx  Workitem associated to this task
      /// \param f    function to execute
      /// \param args arguments for the execution of f
      template<typename InputFunction, typename... Args>
      runner(Workitem* ctx, const InputFunction& f, Args&&... args)
      : AbstractRunner(ctx), f_(std::bind(f, internal::ref<Args&&>::make(args)...))
      { }
      
      /// Steal the work of this runner
      AbstractBoxedFunction * steal(Workitem *ctx) final {
        //Danger: ctx_ could have been already nullified by execute()
        //so the stealer sends it just in case
        return new BoxedFunction<Function>(ctx, std::move(f_));
      }
  
      /// Task execution
      tbb::task* execute() 
      {
        Workitem * const ctx_copy = ctx_.exchange(nullptr);
        
        if (ctx_copy != nullptr) {
          
          if (!ctx_copy->guard_.fetch_add(1)) {
            
            ctx_copy->status = Workitem::Status_t::Running;
            
            Workitem *& ref_father_lcl = enum_thr_spec_father;
            ref_father_lcl = ctx_copy;
            
            f_();
            
            //BBF: in case father thread runs a task.
            //Should be farther checked
            ref_father_lcl = nullptr;
            
            ctx_copy->finish_execution();
            
          } else {
            while (ctx_copy->guard_ < 3) { } //Wait for new BoxedFunction to be built
            ctx_copy->guard_ = 4; // Notify we are leaving
          }
          
        }

        //set_ref_count(1);
    
        return nullptr;
      }
  
      virtual ~runner() {}
  
    };

    /// Argument analysis base case
    template<typename T_it>
    int fill_args(arg_info*&) { return 0; }

    /// Recursively analyze the list of types and arguments, filling in the argument list of the Workitem
    template<typename T_it, typename Head, typename... Tail>
    int fill_args(arg_info*& args, Head&& h, Tail&&... t) {
      typedef typename boost::mpl::deref<T_it>::type curr_t;    //Formal parameter type
      constexpr bool is_reference = std::is_reference<curr_t>::value;
      typedef typename std::remove_reference<curr_t>::type deref_t;
      constexpr bool is_const = std::is_const<deref_t>::value;
      constexpr bool is_writable = is_reference && !is_const;
      constexpr bool is_barray = is_blitz_array<typename std::remove_const<typename std::remove_reference<Head>::type>::type>::value;
      constexpr int process_argument =
      !is_ignored<curr_t&&>::value &&
      !is_ignored<Head&&>::value &&
      !(std::is_rvalue_reference<Head&&>::value && !is_barray);
      
      if(process_argument) {
	
        arg_info* n = arg_info::Pool.malloc();
	n->size = sizeof(Head);
	n->wr = is_writable;
	n->next = nullptr;
	n->fill_in_array(h);
	
	// Insert new_arg (n) in order
	n->insert_in_arglist(args);
      }
      
      return process_argument + fill_args<typename boost::mpl::next<T_it>::type>(args, std::forward<Tail>(t)...);
    }

/// Common steps to all the spawn function versions
#define INLINED_IN_SPAWN(parameter_types)                                                                             \
          internal::arg_info *pargs = nullptr;                                                                        \
          const int nargs = internal::fill_args<typename boost::mpl::begin<parameter_types>::type>(pargs, std::forward<Args>(args)...); \
                                                                                                                      \
          if(!internal::master_task)                                                                                  \
            internal::start_master();                                                                                 \
                                                                                                                      \
          internal::Workitem* w = internal::Workitem::Pool.malloc(pargs, nargs);                                      \
                                                                                                                      \
          w->insert_in_worklist(new (tbb::task::allocate_additional_child_of(*internal::master_task)) internal::runner<decltype(std::bind(f, internal::ref<Args&&>::make(args)...))>(w, f, std::forward<Args>(args)...));                                      \
          /* w->run(); return *w->task; */


/// Data type returned by spawn
typedef void spawn_ret_t;

  } //namespace internal

/// Define the maximum number of ready tasks allowed to be waiting to be executed
/// before the enqueueing thread tries to execute one of them.
///
/// This value only makes sense if the library has been compiled with DEPSPAWN_FAST_START on.
/// By default the library sets this value to twice the number of threads in use.
extern void set_task_queue_limit(int limit) noexcept;

/// Retrieve the value set by set_task_queue_limit() or the environment variable DEPSPAWN_TASK_QUEUE_LIMIT
///or -1 if DepSpawn was not compiled with DEPSPAWN_FAST_START on.
extern int get_task_queue_limit() noexcept;

/// Specify the number of threads and stack size per thread to use.
///
/// If the library has been compiled with DEPSPAWN_FAST_START, it also invokes
/// set_task_queue_limit() asking for two ready tasks per thread, which is the default.
///
/// \param nthreads Number of threads to use.
///                 The value tbb::task_scheduler_init::automatic creates one
///                 thread per hardware thread available, which is the default
///                 behavior when the argument is not specified.
/// \param thread_stack_size Stack size for each thread.
///                 A value 0 specifies the use of the default stack size.
extern void set_threads(int nthreads = tbb::task_scheduler_init::automatic,
                        tbb::stack_size_type thread_stack_size = 0);

/// Retrieve number of threads currently in use
extern int get_num_threads() noexcept;

#ifdef SEQUENTIAL_DEPSPAWN

#define spawn(f, ...) f(__VA_ARGS__)

#else

  /// Spawns a std::function
  template<typename Function, typename... Args>
  internal::spawn_ret_t spawn(const std::function<Function>& f, Args&&... args) {
    typedef boost::function_types::parameter_types<Function> parameter_types;
    INLINED_IN_SPAWN(parameter_types);
  }


  /// Spawns a boost::function
  template<typename Function, typename... Args>
  internal::spawn_ret_t spawn(const boost::function<Function>& f, Args&&... args) {
    typedef boost::function_types::parameter_types<Function> parameter_types;
    INLINED_IN_SPAWN(parameter_types);
  }


  /// Spawns a regular (not lambda, std::function or boost::function) function
  template<typename Function, typename... Args>
  typename std::enable_if< std::is_reference<Function>::value &&
                           std::is_function<typename std::remove_reference<Function>::type>::value, internal::spawn_ret_t >::type
  spawn(Function&& f, Args&&... args) {
    typedef boost::function_types::parameter_types<typename std::remove_reference<Function>::type> parameter_types;
    INLINED_IN_SPAWN(parameter_types);
  }
  
  /// Spawns a lambda function
  template<typename Function, typename... Args>
  typename std::enable_if< ! std::is_reference<Function>::value &&
                           ! std::is_member_function_pointer<Function>::value, internal::spawn_ret_t >::type
  spawn(Function&& fl, Args&&... args) {
    typedef typename internal::function_type<Function>::signature signature_type;
    typedef boost::function_types::parameter_types<signature_type> parameter_types;

    const std::function<signature_type> f = internal::make_function(std::forward<Function>(fl));
    INLINED_IN_SPAWN(parameter_types);
  }

  /// Spawns a pointer to member function
  template<typename Function, typename... Args>
  typename std::enable_if< std::is_member_function_pointer<Function>::value, internal::spawn_ret_t >::type
  spawn(Function f1, Args&&... args) {
    /* with "Function f" : Works fine for gcc 4.7.1 and 5.3.0, but not Apple's clang :((
    typedef boost::function_types::parameter_types<Function> parameter_types;
    */
    typedef typename internal::function_type<Function>::signature signature_type;
    typedef boost::function_types::parameter_types<signature_type> parameter_types;
    
    const std::function<signature_type> f = internal::make_function(std::forward<Function>(f1));
    INLINED_IN_SPAWN(parameter_types);
  }

  /// Spawns a functor, but only if there is a single operator()
  template<typename T, typename... Args>
  typename std::enable_if< std::is_reference<T>::value &&
                         ! std::is_member_function_pointer<typename std::remove_reference<T>::type>::value &&
                         ! std::is_function<typename std::remove_reference<T>::type>::value &&
                         ! internal::is_function_object<typename std::remove_reference<T>::type>::value, internal::spawn_ret_t >::type
  spawn(T&& functor, Args&&... args) {
    typedef typename std::remove_reference<T>::type base_type;
    return spawn(& base_type::operator(), std::forward<T>(functor), std::forward<Args>(args)...);
  }

#endif // SEQUENTIAL_DEPSPAWN

  /// Waits for all tasks to finish
  void wait_for_all();

  /// Waits for all the subtasks of the current task to finish
  /// \param priority if true, the thread will only execute subtasks it depends on
  /// during the wait. Otherwise, if it is idle because all those tasks are already
  /// running or waiting for dependencies, it can run other tasks while it waits.
  void wait_for_subtasks(bool priority = true);

  /// \brief Wait for a specific group of variables to be written
  ///
  /// Notice that since this function only waits for writes, there could be active tasks
  ///reading these variables when this function returns to the caller.
  /// \param args  arguments to wait for
  template<typename... Args>
  void wait_for(const Args&... args) {

    if(!internal::master_task) //There should be nothing to wait for
      return;
    
    typedef void Function(const Args&... args);
    //std::function<Function> f = [](const Args&... args) { std::cout << "EXEC!\n"; };
    typedef boost::function_types::parameter_types<Function> parameter_types;
    internal::arg_info *pargs = nullptr;
    const int nargs = internal::fill_args<typename boost::mpl::begin<parameter_types>::type>(pargs, args...);
    
    /*
    tbb::task* const dummy = new (tbb::task::allocate_root()) tbb::empty_task;
    dummy->set_ref_count(1 + 1);
    
    internal::Workitem* w = internal::Workitem::Pool.malloc(pargs);
    // w->insert_in_worklist(new (dummy->allocate_child()) internal::runner<decltype(std::bind(f, args...))>(w, f, args...));
    w->insert_in_worklist(new (dummy->allocate_child()) internal::runner<decltype(FV)>(w, FV));
    
    dummy->wait_for_all();
    dummy->destroy(*dummy);
    */
    
    internal::common_wait_for(pargs, nargs);
  }

  /// \brief When destroyed, it makes sure that all the tasks spawned by the current
  ///thread since its creation have finished.
  ///
  /// A typical pattern of use for an Observer is \code
  /// { Observer o;
  ///   spawn(...);
  ///   spawn(...);
  /// }
  /// \endcode
  /// which makes sure that the two spawned tasks have finished before leaving the block.
  class Observer {
    
    /// most recent workitem spawned when the Observer is built
    internal::Workitem * const limit_;
    
    /// parent workitem of the task in which the Observer is built
    internal::Workitem * const cur_father_;
    
    /// During the wait we will execute tasks outside the critical region if there is no priority_
    const bool priority_;
    
    /// Wait for all subtasks of \c w
    ///
    /// Currently only used by wait_for_subtasks()
    /// \param priority has the same meaning as in the main constructor
    /// \param w        task whose subchildren we want to wait for
    Observer(bool priority, internal::Workitem *w);
    
    friend void wait_for_subtasks(bool priority);
    
  public:
    
    /// Constructor
    /// \param priority if true, the observer will only collaborate in the execution of pending
    /// tasks found in the critical path since its creation. Otherwise, if it is idle because
    /// all those tasks are already running or waiting for dependencies, it can run
    /// other tasks while it waits.
    Observer(bool priority = true);
    
    ~Observer();
    
  };

/* This helps expand __LINE__ so that each Observer has a different variable name,
   although this does not happen if we put two depspawn_sync in the same line.
   But this does not matter because they would be necessaritly nested, so actuallly
   all the depspawn_sync could have exactly the same name. This looks just nicer. */
#define depspawnBottomTOKENPASTE(x, y) x ## y
#define depspawnTOKENPASTE(x, y) depspawnBottomTOKENPASTE(x, y)

/// Executes a block of code and waits for all its subtasks to finish
#define depspawn_sync(...) { depspawn::Observer depspawnTOKENPASTE(__depspawn_temporary, __LINE__);\
                             {__VA_ARGS__;} }

#ifdef DEPSPAWN_BLITZ

  /// Parallel for based on a 1D Blitz++ range
  /// \param r  Blitz++ range to run in parallel
  /// \param f  function to execute in parallel. Its only input must be a chunk of t
  /// \param bs optional step
  template<typename F>
  void for_range(const blitz::Range& r, const F& f, int bs = -1) {
    const int num_threads = get_num_threads();
    
    if(bs == -1) bs = (r.length() + num_threads - 1) / (num_threads);
    int lbs;
    int i;
    for(i = r.first(); i < r.last(); i+=bs) {
      lbs = r.last() -i +1 <= bs ? r.last() -i +1 : bs;
      f(blitz::Range(i, i+lbs-1));
    }
  }
  
  /// Parallel for based on two 1D Blitz++ ranges
  /// \param r1  Blitz++ range to run in parallel (first dimension)
  /// \param r2  Blitz++ range to run in parallel (second dimension)
  /// \param f   function to execute in parallel. Its inputs must be a chunk of r1 and a chunk of r2
  /// \param bs1 optional step for the first dimension
  /// \param bs2 optional step for the second dimension
  template<typename F>
  void for_range(const blitz::Range& r1, const blitz::Range& r2, const F& f, int bs1 = -1, int bs2 = -1) {
    const int num_threads = get_num_threads();

    if(bs1 == -1) {
      bs1 = (r1.length()  + num_threads - 1) / (num_threads);
    }
    if(bs2 == -1) {
      bs2 = (r2.length() + num_threads - 1) / (num_threads);
    }
    int lbs1, lbs2;
    int i, j;
    for(i = r1.first(); i < r1.last(); i+=bs1) {
      lbs1 = r1.last() -i +1 <= bs1 ? r1.last() -i +1 : bs1;
      for(j = r2.first(); j < r2.last(); j+=bs2) {
	lbs2 = r2.last() -j +1 <= bs2 ? r2.last() -j +1 : bs2;
	f(blitz::Range(i, i+lbs1-1), blitz::Range(j, j+lbs2-1));
      }
    }
  }

#endif // DEPSPAWN_BLITZ

} //namespace depspawn


#ifdef DEPSPAWN_BLITZ

#define DEFINE_NEW_ARRAY_TYPE(type, dim) \
namespace depspawn { \
  namespace internal { \
    template<> \
    struct is_blitz_array<blitz::Array<type, dim>> : public boost::true_type {}; \
  }; \
};

DEFINE_NEW_ARRAY_TYPE(int, 1);
DEFINE_NEW_ARRAY_TYPE(int, 2);
DEFINE_NEW_ARRAY_TYPE(int, 3);
DEFINE_NEW_ARRAY_TYPE(int, 4);
DEFINE_NEW_ARRAY_TYPE(unsigned int, 1);
DEFINE_NEW_ARRAY_TYPE(unsigned int, 2);
DEFINE_NEW_ARRAY_TYPE(unsigned int, 3);
DEFINE_NEW_ARRAY_TYPE(unsigned int, 4);
DEFINE_NEW_ARRAY_TYPE(unsigned long long, 1);
DEFINE_NEW_ARRAY_TYPE(unsigned long long, 2);
DEFINE_NEW_ARRAY_TYPE(unsigned long long, 3);
DEFINE_NEW_ARRAY_TYPE(unsigned long long, 4);
DEFINE_NEW_ARRAY_TYPE(long long, 1);
DEFINE_NEW_ARRAY_TYPE(long long, 2);
DEFINE_NEW_ARRAY_TYPE(long long, 3);
DEFINE_NEW_ARRAY_TYPE(long long, 4);
DEFINE_NEW_ARRAY_TYPE(float, 1);
DEFINE_NEW_ARRAY_TYPE(float, 2);
DEFINE_NEW_ARRAY_TYPE(float, 3);
DEFINE_NEW_ARRAY_TYPE(float, 4);
DEFINE_NEW_ARRAY_TYPE(double, 1);
DEFINE_NEW_ARRAY_TYPE(double, 2);
DEFINE_NEW_ARRAY_TYPE(double, 3);
DEFINE_NEW_ARRAY_TYPE(double, 4);
DEFINE_NEW_ARRAY_TYPE(long double, 1);
DEFINE_NEW_ARRAY_TYPE(long double, 2);
DEFINE_NEW_ARRAY_TYPE(long double, 3);
DEFINE_NEW_ARRAY_TYPE(long double, 4);
#endif // DEPSPAWN_BLITZ


#endif // __DEPSPAWN_H
