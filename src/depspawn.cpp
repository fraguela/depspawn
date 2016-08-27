/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2016 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 This file is part of DepSpawn.
 
 DepSpawn is free software; you can redistribute it and/or modify it under the terms
 of the GNU General Public License as published by the Free Software Foundation;
 either version 2 as published by the Free Software Foundation.
 
 DepSpawn is distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with Threading Building Blocks; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 As a special exception, you may use this file as part of a free software
 library without restriction.  Specifically, if other files instantiate
 templates or use macros or inline functions from this file, or you compile
 this file and link it with other files to produce an executable, this
 file does not by itself cause the resulting executable to be covered by
 the GNU General Public License.  This exception does not however
 invalidate any other reasons why the executable file might be covered by
 the GNU General Public License.
*/

///
/// \file     depspawn.cpp
/// \brief    Main implementation file for DepSpawn together with workitem.cpp
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#include <cstdlib>
#include <climits>
#include <cstdio>
#include <cassert>
#include <unordered_set>
#include <tbb/tbb_thread.h>
#include "depspawn/depspawn_utils.h"
#include "depspawn/depspawn.h"
#ifdef DEPSPAWN_PROFILE
#include "tbb/tick_count.h"
#endif



namespace {

  using namespace depspawn::internal;
  
  tbb::atomic<Workitem*> worklist;
  tbb::atomic<bool> eraser_assigned;
  tbb::atomic<int> ObserversAtWork;
  std::function<void(void)> FV = [](){};
  
  DEPSPAWN_PROFILEDEFINITION(tbb::atomic<unsigned int>
                             profile_jobs = 0,
                             profile_steals = 0,
                             profile_steal_attempts = 0,
                             profile_early_terminations = 0);
  
  DEPSPAWN_PROFILEDEFINITION(tbb::atomic<unsigned long long int>
                             profile_workitems_in_list = 0,
                             profile_workitems_in_list_active = 0,
                             profile_workitems_in_list_early_termination = 0,
                             profile_workitems_in_list_active_early_termination = 0);
  
  DEPSPAWN_PROFILEDEFINITION(unsigned int profile_erases = 0);
  
  DEPSPAWN_PROFILEDEFINITION(double profile_time_eraser_waiting = 0.);

  /// Returns true if the intervals [s1, e1] and [s2, e2] overlap
  template<typename T>
  constexpr bool overlaps_intervals(const T s1, const T e1, const T s2, const T e2)
  {
    return (s1 <= s2) ? (s2 <= e1) : (s1 <= e2);
  }
  
  constexpr bool overlaps(const arg_info* const a, const arg_info* const b)
  {
    return overlaps_intervals(a->addr, a->addr + a->size - 1,
                           b->addr, b->addr + b->size - 1);
  }
  
  /// Returns true if the interval [s1, e1] contains the interval [s2, e2]
  template<typename T>
  constexpr bool contains_intervals(const T s1, const T e1, const T s2, const T e2)
  {
    return (s1 <= s2) && (e2 <= e1);
  }
  
  /// Means a contains b
  constexpr bool contains(const arg_info* const a, const arg_info* const b)
  { //the -1 for the end would be correct, but unnecessary in this test
    return contains_intervals(a->addr, a->addr + a->size, b->addr, b->addr + b->size);
  }
  
  /// Tries to run the Workitem work
  ///
  /// This only happens if it is in Ready state
  /// \return wheter it could actually run the Workitem
  bool try_to_run(Workitem * w)
  {
    bool success = false;
    if ( w->status == Workitem::Status_t::Ready ) {
      AbstractBoxedFunction * const stolen_abf = w->steal();
      if (stolen_abf != nullptr) {
        ObserversAtWork.fetch_and_decrement();
        success = true;
        stolen_abf->run_in_env(true);
        delete stolen_abf;
      }
    }
    return success;
  }
  
  DEPSPAWN_PROFILEDEFINITION(
      void profile_display_results(bool reset = false) {
        // == spawns
        unsigned jobs = (unsigned)profile_jobs;
        
        unsigned avg_wil, avg_wilt;
        if(jobs) {
          // workitems in worklist per jobs spawned
          avg_wil = (unsigned long long)profile_workitems_in_list / (unsigned long long)jobs;
          // tested/active (not done or deallocatable) workitems in worklist per jobs spawned
          avg_wilt = (unsigned long long)profile_workitems_in_list_active / (unsigned long long)jobs;
        } else {
          avg_wil = avg_wilt = 0;
        }
        
        unsigned failed_steals = (unsigned)profile_steal_attempts - (unsigned)profile_steals;
        
        unsigned early_terminations = (unsigned)profile_early_terminations;
        unsigned avg_wil_early, avg_wilt_early;
        if(early_terminations) {
          avg_wil_early = (unsigned long long)profile_workitems_in_list_early_termination / early_terminations;
          avg_wilt_early = (unsigned long long)profile_workitems_in_list_active_early_termination / early_terminations;
        } else {
          avg_wil_early = avg_wilt_early = 0;
        }
        
        printf("Jobs: %u Steals=%u FailedSteals=%u Erases=%u (%lf s.) Wil/job=%u Wilt/job=%u\n",
               jobs, (unsigned)profile_steals, failed_steals,
               profile_erases, profile_time_eraser_waiting,
               avg_wil, avg_wilt);
        printf("Early Term: %u Wil/e.t.=%u Wilt/e.t.=%u\n",
               early_terminations, avg_wil_early, avg_wilt_early);
        /*
        printf("Wil =%llu\nWilt=%llu\nWil_e =%llu\nWilt_e=%llu\n",
               (unsigned long long)profile_workitems_in_list, (unsigned long long)profile_workitems_in_list_active,
               (unsigned long long)profile_workitems_in_list_early_termination, (unsigned long long)profile_workitems_in_list_active_early_termination);
         */
        if(reset) {
          profile_jobs = profile_steals = profile_steal_attempts = profile_early_terminations = 0;
          profile_workitems_in_list = profile_workitems_in_list_active = 0;
          profile_workitems_in_list_early_termination = profile_workitems_in_list_active_early_termination = 0;
          profile_erases = 0;
          profile_time_eraser_waiting = 0.;
        }
      }
  ) // END DEPSPAWN_PROFILEDEFINITION
  
};


namespace depspawn {
  
  namespace internal {

    tbb::task* master_task = 0;
    tbb::enumerable_thread_specific<Workitem *> enum_thr_spec_father;

    /// \brief Number of threads in use
    ///
    /// It is only accurate if the user has initialized the library with set_threads().
    ///Otherwise it just estimates that there is one SW thread per HW thread.
    int Nthreads = tbb::tbb_thread::hardware_concurrency();

#ifndef DEPSPAWN_POOL_CHUNK_SZ
    ///Number of elements to allocate at once in each new allocation requested by the pools
#define DEPSPAWN_POOL_CHUNK_SZ 32
#endif
    
#ifndef DEPSPAWN_MIN_POOL_ITEM_SZ
///Minimum space to allocate for each item in the pool
#define DEPSPAWN_MIN_POOL_ITEM_SZ 0
#endif
    
    Pool_t<arg_info, DEPSPAWN_SCALABLE_POOL> arg_info::Pool(DEPSPAWN_POOL_CHUNK_SZ, DEPSPAWN_MIN_POOL_ITEM_SZ);
    Pool_t<Workitem, DEPSPAWN_SCALABLE_POOL> Workitem::Pool(DEPSPAWN_POOL_CHUNK_SZ, DEPSPAWN_MIN_POOL_ITEM_SZ);
    Pool_t<Workitem::_dep, DEPSPAWN_SCALABLE_POOL> Workitem::_dep::Pool(DEPSPAWN_POOL_CHUNK_SZ, DEPSPAWN_MIN_POOL_ITEM_SZ);
    
#ifdef DEPSPAWN_FAST_START
    // They are exportable for derived libraries (could be static for DepSpawn itself)
    static const int FAST_ARR_SZ  = 16;
    int FAST_THRESHOLD = tbb::tbb_thread::hardware_concurrency() * 2;
#endif

    int getNumThreads()
    {
      return Nthreads;
    }
    
    void start_master()
    {
      worklist = nullptr;
      eraser_assigned = false;
      ObserversAtWork = 0;
      master_task = new (tbb::task::allocate_root()) tbb::empty_task;
      master_task->set_ref_count(1);
      enum_thr_spec_father.local() = nullptr;
    }
    
    void common_wait_for(arg_info *pargs, int nargs)
    {
      tbb::task* const dummy = new (tbb::task::allocate_root()) tbb::empty_task;
      dummy->set_ref_count(1 + 1);
      
      internal::Workitem* w = internal::Workitem::Pool.malloc(pargs, nargs);
      w->insert_in_worklist(new (dummy->allocate_child()) internal::runner<decltype(FV)>(w, FV));
      
      dummy->wait_for_all();
      dummy->destroy(*dummy);
    }
    
    ///TODO: This implementation is ONLY for general memory areas. Arrays are inserted replicated
    void arg_info::solve_overlap(arg_info *other)
    { size_t tmp_size;
      
      //fprintf(stderr, "%zu [%d %zu] [d %zu]\n", addr, other->wr, other->size, wr, size);
      
      const bool orwr = wr || other->wr;
      
      if(size == other->size) {
	other->wr = orwr;
	Pool.free(this);
      } else if (size < other->size) {
	
	if(orwr == other->wr)
	  Pool.free(this);
	else {
	  tmp_size = size;
	  
	  addr += size;
	  size = other->size - size;
	  wr = other->wr; //should be false
	  
	  other->size = tmp_size;
	  other->wr = orwr; //should be true
	  
	  insert_in_arglist(other);
	}
	
      } else { //size > other->size
	
	if(orwr == wr) {
	  other->wr = orwr;
	  other->size = size;
	  Pool.free(this);
	} else { //Should be other->wr = true, this->wr = false
	  tmp_size = other->size;
	  
	  addr += tmp_size;
	  size -= tmp_size;
	  
	  insert_in_arglist(other);
	}
	
      }
      
    }
    
    void arg_info::insert_in_arglist(arg_info*& args)
    { arg_info *p, *prev;
      
      if(!args) {
	args = this;
      } else if(addr < args->addr) {
	next = args;
	args = this;
      } else {
	
	prev = args;
	p = args->next;
	while(p) {
	  if(addr < p->addr) {
	    if( !is_array() && (addr == prev->addr) ) //Arrays are inserted replicated
	      solve_overlap(prev);
	    else {
	      prev->next = this;
	      next = p;
	    }
	    break;
	  }
	  prev = p;
	  p = p->next;
	}
	
        if(!p) {
          if( !is_array() && (addr == prev->addr) ) { //Arrays are inserted replicated
	    solve_overlap(prev);
          } else {
	    prev->next = this;
          }
        }
      }
      
    }
    
    bool arg_info::overlap_array(const arg_info *other) const
    { int i;

      const int lim = rank;
      
      do {
	
	if(wr || other->wr) {
	  //if(data != other->data) return false; SHOULD be ==
	  
	  assert(lim == other->rank);
	  
	  for(i = 0; i < lim; i++) {
	    if(!overlaps_intervals(std::get<0>(array_range[i]), std::get<1>(array_range[i]),
				   std::get<0>(other->array_range[i]), std::get<1>(other->array_range[i])))
	      break;
	  }
	  
	  if(i == lim) {
	    //if(depspawn::debug) std::cout << "DEP FOUND\n";
	    return true;
	  }
	}
	
	other = other->next;
	
      } while(other && (addr == other->addr));
      
      return false;
    }
    
    bool arg_info::is_contained_array(const arg_info *other) const
    { int i;
      
      const int lim = rank;
      
      do {

          assert(lim == other->rank);
          
          for(i = 0; i < lim; i++) {
            if(!contains_intervals(std::get<0>(other->array_range[i]), std::get<1>(other->array_range[i]),
                                   std::get<0>(array_range[i]), std::get<1>(array_range[i])))
              break;
          }
          
          if(i == lim) {
            //if(depspawn::debug) std::cout << "DEP FOUND\n";
            return true;
          }

        
        other = other->next;
        
      } while(other && (addr == other->addr));
      
      return false;
    }
    
    bool is_contained(const Workitem* const w, const Workitem * const ancestor)
    {
      const arg_info *arg_w = w->args;
      const arg_info *arg_p = ancestor->args;
      
      while(arg_p && arg_w) {
        
        if (arg_w->addr < arg_p->addr) {
          return false;
        }
        
        if (arg_w->is_array()) {
          if(arg_p->addr == arg_w->addr) {
            if(!arg_w->is_contained_array(arg_p))
              return false;
            //Same array. Was contained => progress w
            arg_w = arg_w->next;
          } else { // Different array, arg_w->addr > arg_p->addr => progress p
            arg_p = arg_p->next;
          }
        } else { //Here arg_p->addr <= arg_w->addr
          if ((arg_w->addr + arg_w->size) <= (arg_p->addr + arg_p->size) ) {
            arg_w = arg_w->next;
          } else {
            arg_p = arg_p->next;
          }
        }
        
      }
      
      return arg_w == nullptr;
    }
    
    
    void AbstractBoxedFunction::run_in_env(bool from_wait) {
      
      Workitem *& ref_father_lcl = enum_thr_spec_father.local();
      Workitem *  cur_father_lcl = ref_father_lcl;
      
      ctx_->status = Workitem::Status_t::Running;
      
      ref_father_lcl = ctx_;
      
      this->run();

      if (from_wait) {
        ObserversAtWork.fetch_and_increment();
        while (eraser_assigned) {
          // Wait for current eraser, if any, to finish
        }
      }
      
      ctx_->finish_execution();
      
      ref_father_lcl = cur_father_lcl;
      
    }
    
DEPSPAWN_DEBUGDEFINITION(
    /// Internal debugging purposes
    //  expr -a 0 -- depspawn::internal::debug_follow_list(worklist.my_storage.my_value, false)
    void debug_follow_list(Workitem *p, bool doprint) {
      static const int Nstates = (int)Workitem::Status_t::Deallocatable + 1;
      unsigned int stath[Nstates];
      unsigned int n = 0;
      
      for (int i = 0; i < Nstates; i++) {
        stath[i] = 0;
      }
      
      while (p != nullptr) {
        n++;
        stath[(int)(p->status)]++;
        if (doprint) {
          printf("%p (%d)\n", p, (int)(p->status));
        }
        p = p->next;
      }
      
      printf("%u\n", n);
      for (int i = 0; i < Nstates; i++) {
         printf("stat %d -> %u\n", i, stath[i]);
      }
    }
) // END DEPSPAWN_DEBUGDEFINITION
    
  } //namespace internal

  void wait_for_all()
  { Workitem *p, *dp;

    if(internal::master_task) {
      internal::master_task->wait_for_all();
      tbb::task::destroy(*internal::master_task);
      internal::master_task = nullptr;
      
      if(worklist != nullptr) {
        for(p = worklist; p; p = p->next) {
          dp = p;
        }
        internal::Workitem::Pool.freeLinkedList(worklist, dp);
      }
      
      worklist = nullptr;
      
      DEPSPAWN_PROFILEACTION(profile_display_results(true));
    }

  }
  
  void wait_for_subtasks(bool priority)
  {
    internal::Workitem * cur_father = enum_thr_spec_father.local();
    
    //printf(" WFS %s\n", cur_father == nullptr ? "0" : "nested");
    
    if (cur_father == nullptr) {
      wait_for_all();
    } else {
      Observer o(priority, cur_father);
    }
  }
  
  void set_task_queue_limit(int limit)
  {
#ifdef DEPSPAWN_FAST_START
    internal::FAST_THRESHOLD = limit;
#endif
  }
  
  void set_threads(int nthreads, tbb::stack_size_type thread_stack_size)
  { static tbb::task_scheduler_init * Scheduler = nullptr;
    
    if (nthreads == tbb::task_scheduler_init::deferred) {
      printf("set_threads(tbb::task_scheduler_init::deferred) unsupported\n");
      exit(EXIT_FAILURE);
    }
    
    if (Scheduler != nullptr) {
      delete Scheduler;
    }
    
    Scheduler = new tbb::task_scheduler_init(nthreads, thread_stack_size);
    assert(Scheduler != nullptr);

    if (nthreads == tbb::task_scheduler_init::automatic) {
      nthreads = tbb::tbb_thread::hardware_concurrency(); //Reasonable estimation
    }

    Nthreads = nthreads;
    
    set_task_queue_limit(2 * nthreads); //Heuristic
  }

  Observer::Observer(bool priority) :
  limit_(worklist),
  cur_father_(enum_thr_spec_father.local()),
  priority_(priority)
  { }
  
  Observer::Observer(bool priority, internal::Workitem *w) :
  limit_(w),
  cur_father_(w),
  priority_(priority)
  { }
  
  Observer::~Observer()
  { std::unordered_set<internal::Workitem *> fathers({cur_father_});
    internal::Workitem *p;
    bool must_reiterate;
    
    ObserversAtWork.fetch_and_increment(); //disable future attempts to erase Workitems during my activity
    while (eraser_assigned) {
      // Wait for current eraser, if any, to finish
    }
    
    /* if there was a cur_father_, since we are inside it, and limit_ is it or more recent,
     limit_ cannot have been deallocated, so limit_ is a safe limit.
     
     if cur_father_=null and limit_ was deallocated, all previous tasks finished,
     and so we have to execute everything up to the end of the list, which is cur_father_=null.
     
     if cur_father_=null and limit_ was not deallocated, we have to execute
     everything up to limit_, but it could have been deallocated, so null is a safe limit,
     although we are waiting for more task that we should actually do.
     
     Also it is interesting to notice that no Workitem in the high-level critical path can
     be Filling, but Workitems descended from them could.
     */
    
    internal::Workitem * const safe_end = (cur_father_ != nullptr) ? limit_ : nullptr;
    //printf("%p w until %p!=NULL->%p\n", enum_thr_spec_father.local(), cur_father_, limit_);

    //The first round always has priority, i.e., is only devoted to the critical path
    bool priority = true;

    do {
      must_reiterate = false;
      bool helped = false;
  
      // We must begin from worklist because new work we must do may have been spawned
      for (p = worklist; p != safe_end; p = p->next) {
        if (fathers.find(p->father) != fathers.end()) { //Workitem in the critical path
          fathers.insert(p); //it could be the father of other tasks that would have to finish
          if (p->status < Workitem::Status_t::Done) {
            must_reiterate = true;
            bool helped_here = try_to_run(p);
            helped = helped || helped_here;
          }
        } else {
          if (!priority) {
            try_to_run(p);
          }
        }
      }
      
      // While we are able to help with tasks in the critical path, we remain focused on it.
      // Otherwise we adopt the priority specified by the user.
      priority = helped ? priority : priority_;
      
    } while (must_reiterate);
    
    //printf("%p exit\n", enum_thr_spec_father.local());
    
    ObserversAtWork.fetch_and_decrement(); //I'm done
  }
  
} //namespace depspawn


#include "workitem.cpp"


