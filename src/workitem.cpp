/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2021 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Distributed under the MIT License. (See accompanying file LICENSE)
*/

///
/// \file     workitem.cpp
/// \brief    Implementation of Workitem
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#include <vector>

namespace depspawn {
  
  namespace internal {

    Workitem::Workitem(arg_info *iargs, int nargs) :
    status(Status_t::Filling), optFlags_(0),
    guard_(0), nargs_(static_cast<char>(nargs)),
    args(iargs), next(nullptr), father(enum_thr_spec_father),
    ndependencies(0), nchildren(1),
    task(nullptr), deps(nullptr), lastdep(nullptr)
    {
      if(father)
        father->nchildren.fetch_add(1);
    }

#ifdef DEPSPAWN_USE_TBB
    AbstractBoxedFunction * Workitem::steal() {
      AbstractBoxedFunction * ret = nullptr;
      if( (status == Status_t::Ready) && (!guard_.fetch_add(1)) ) {
        ret = task->steal(this);
        //Cancel task?
        Workitem * const ctx_copy = task->ctx_.exchange(nullptr); //After this the task may have been deallocated
        if (ctx_copy == nullptr) { //if task began execution
          guard_.fetch_add(1); // Notify we have copied and are ready to go
          while (guard_ < 4) { } //Wait for task to notify its acknowledgement to ours
        }
        // guard_ = 3; //After this the task may have been deallocated
        //There is a *totally minimal* chance that after this point, even after we waited for the
        //guard_ to be 2 if (ctx_copy == nullptr), the task goes to sleep and this workitem is
        //deallocated, breaking the task access to it.
      }
      return ret;
    }
#endif

    void Workitem::insert_in_worklist(AbstractRunner* itask)
    { Workitem* p;
      arg_info *arg_p, *arg_w;

      //Save original list of arguments
      int nargs = static_cast<int>(nargs_);
      arg_info* argv[nargs+1];
      arg_w = args;
      for (int i = 0; arg_w != nullptr; i++) {
        argv[i] = arg_w;     //printf("Fill %d %lu\n", i, arg_w->addr);
        arg_w = arg_w->next;
      }
      argv[nargs] = nullptr;
      
      DEPSPAWN_PROFILEDEFINITION(unsigned int profile_workitems_in_list_lcl = 0,
                                              profile_workitems_in_list_active_lcl = 0);
      DEPSPAWN_PROFILEDEFINITION(bool profile_early_termination_lcl = false);
      
      DEPSPAWN_PROFILEACTION(profile_jobs++);
      
#ifdef DEPSPAWN_FAST_START
#ifdef DEPSPAWN_USE_TBB
      AbstractBoxedFunction *stolen_abf = nullptr;
      static constexpr int FAST_ARR_SZ  = 16;
      Workitem* fast_arr[FAST_ARR_SZ];
#else
      bool steal_work = false;
#endif
      int nready = 0, nfillwait = 0;
#endif
      
      task = itask;
      
      Workitem* ancestor = father;

      next = worklist.load(std::memory_order_relaxed);
      while(!worklist.compare_exchange_weak(next, this));

      for(p = next; p != nullptr; p = p->next) {

        DEPSPAWN_PROFILEACTION(profile_workitems_in_list_lcl++);
        
        // sometimes p == this, even if it shouldn't
        if(p == this) { // fprintf(stderr, "myself?\n");
          continue; // FIXIT
        }
        if(p == ancestor) {
          ancestor = p->father;
          /// TODO: is_contained should be adapted to use argv
          if ( (ancestor != nullptr) && is_contained(this, ancestor)) {
            DEPSPAWN_PROFILEACTION(profile_early_termination_lcl = true);
            optFlags_ |= OptFlags::FatherScape;
            break;
          }
        } else {
          const auto tmp_status = p->status;
          if(tmp_status < Status_t::Done) {
            DEPSPAWN_PROFILEACTION(profile_workitems_in_list_active_lcl++);
            int arg_w_i = 0;
            arg_p = p->args; // preexisting workitem
            arg_w = argv[0]; // New workitem
            while(arg_p && arg_w) {

              const bool conflict = arg_w->is_array()
              ? ( (arg_p->addr == arg_w->addr) && arg_w->overlap_array(arg_p) )
              : ( (arg_p->wr || arg_w->wr) && overlaps(arg_p, arg_w) );
              
              if(conflict) { // Found a dependency
                ndependencies++;
                Workitem::_dep* newdep = Workitem::_dep::Pool.malloc();
                newdep->next = nullptr;
                newdep->w = this;
                
                while (p->deps_mutex_.test_and_set(std::memory_order_acquire))  // acquire lock
                  ; // spin
                
                if(!p->deps)
                  p->deps = p->lastdep = newdep;
                else {
                  p->lastdep->next = newdep;
                  p->lastdep = p->lastdep->next;
                }

                //lock.release();
                p->deps_mutex_.clear(std::memory_order_release);               // release lock
                
                DEPSPAWN_DEBUGACTION(
                                     /* You can be linking to Done's that are waiting for you to Fill-in
                                       but NOT for Deallocatable Workitems */
                                     if(p->status == Status_t::Deallocatable) {
                                       printf("%p -> Deallocatable %p (was %d)", this, p, (int)tmp_status);
                                       assert(false);
                                     }
                                     ); // END DEPSPAWN_DEBUGACTION
                if (arg_p->wr &&
                    (arg_w->is_array() ? arg_w->is_contained_array(arg_p) : contains(arg_p, arg_w))) {
                  nargs--;
                  /*
                  if (!nargs) {
                    DEPSPAWN_PROFILEACTION(profile_early_termination_lcl = true); //not true actually...
                    // The optimal thing would be to just make this goto to leave the main loop and insert
                    // the Workitem waiting. But in tests with repeated spawns this leads to very fast insertion
                    // that slows down the performance. Other advantages of continuing down the list:
                    // - More likely DEPSPAWN_FAST_START is triggered and it uses oldest tasks
                    // - optFlags_ is correctly computed
      
                    //goto OUT_MAIN_insert_in_worklist_LOOP;
      
                      argv[0] = nullptr;
                    //break; //There is another break anyway there down
                  } else {
                   */
                    for (int i = arg_w_i; i <= nargs; i++) {
                      argv[i] = argv[i+1];
                    }
                  /*}*/
                }
                break;
              } else {
                if(arg_p->addr < arg_w->addr) {
                  arg_p = arg_p->next;
                } else {
                  arg_w = argv[++arg_w_i];
                }
              }
            }
            
#ifdef DEPSPAWN_FAST_START
            const auto tmp_status = p->status;
            nfillwait += ( tmp_status < Status_t::Ready );
            if ( tmp_status == Status_t::Ready ) {
#ifdef DEPSPAWN_USE_TBB
              fast_arr[nready & (FAST_ARR_SZ - 1)] = p;
#endif
              nready++;
            }
#endif
            if( p->status == Status_t::Filling ) {
              optFlags_ |=  OptFlags::PendingFills;
            }
          }
        }
      }
      
//OUT_MAIN_insert_in_worklist_LOOP:
      
#ifdef DEPSPAWN_FAST_START
      if ((nready > FAST_THRESHOLD) || ((nready > Nthreads) && (nfillwait > FAST_THRESHOLD))) {
        DEPSPAWN_PROFILEACTION(profile_steal_attempts++);
#ifdef DEPSPAWN_USE_TBB
        nready = (nready - 1) & (FAST_ARR_SZ - 1);
        do {
          p = fast_arr[nready--];
          stolen_abf = p->steal();
        } while ( (stolen_abf == nullptr) && (nready >= 0) );
#else
        steal_work = true;
#endif
      }
#endif

      // This is set after stealing work because this way
      // the fast_arr workitems should not have been deallocated
      //status = (!ndependencies) ? Ready : Waiting;
      if (!ndependencies) {
        post();
      } else {
        status = Status_t::Waiting;
      }

#ifdef DEPSPAWN_FAST_START
#ifdef DEPSPAWN_USE_TBB
      if (stolen_abf != nullptr) {
        DEPSPAWN_PROFILEACTION(profile_steals++);
        stolen_abf->run_in_env(false);
        delete stolen_abf;
      }
#else
      if(steal_work && TP->try_run()) {
        DEPSPAWN_PROFILEACTION(profile_steals++);
      }
#endif
#endif
      
      DEPSPAWN_PROFILEACTION(
                             if(profile_early_termination_lcl) {
                               profile_early_terminations++;
                               profile_workitems_in_list_early_termination += profile_workitems_in_list_lcl;
                               profile_workitems_in_list_active_early_termination += profile_workitems_in_list_active_lcl;
                             }
                             profile_workitems_in_list += profile_workitems_in_list_lcl;
                             profile_workitems_in_list_active += profile_workitems_in_list_active_lcl;
                             );
    }

    void Workitem::finish_execution()
    { Workitem *p, *worklist_wait_hint;

      if(nchildren.fetch_sub(1) != 1)
        return;
      
      bool erase = false;
      Workitem *current = this;

      do {

        // Finish work
        current->status = Status_t::Done;
      
        /* Without this fence the change of status can be unseen by Workitems that are Filling in */
        std::atomic_thread_fence(std::memory_order_seq_cst);
      
        erase = erase || ((((((intptr_t)current)>>8)&0xfff) < 32) && !ObserversAtWork && !eraser_assigned && eraser_assigned.compare_exchange_weak(erase, true));
      
        worklist_wait_hint = worklist;
        //lastkeep = worklist_wait_hint;
        
        for(p = worklist_wait_hint; p && p != this; p = p->next) { //wait until this, not current
      
          while(p->status == Status_t::Filling) {}
      
          //if(p->status != Deallocatable)
          //  lastkeep = p;
      
          if(!(p->optFlags_ & OptFlags::PendingFills)) {
            if(p->optFlags_ & OptFlags::FatherScape) {
              if (p->status < Status_t::Done) { //The father of a Done/Deallocated could be freed
                p = p->father; //the iterator sets p = p->next;
              }
            } else {
              break;
            }
          }
          
        }
        
        // free lists
        //delete ctx_->task;
      
        Workitem::_dep *idep = current->deps;
        if (idep != nullptr) {
          do {
            if(idep->w->ndependencies.fetch_sub(1) == 1) {
              idep->w->post();
            }
            idep = idep->next;
          } while (idep != nullptr);
          Workitem::_dep::Pool.freeLinkedList(current->deps, current->lastdep);
          current->deps = current->lastdep = nullptr;
        }

        if (current->args != nullptr) {
          arg_info::Pool.freeLinkedList(current->args, [](arg_info *i) { if(i->is_array()) delete [] i->array_range; } );
          current->args = nullptr;
        }
      
        p = current->father;
      
        current->status = Status_t::Deallocatable;
      
        current = p;

      } while ((p != nullptr) && (p->nchildren.fetch_sub(1) == 1));
      
      if(erase) {
        Clean_worklist(worklist_wait_hint);
      }
      
    }
    
    void Workitem::Clean_worklist(Workitem *worklist_wait_hint)
    { static std::vector<Workitem *> Dones;
      static std::vector< std::pair<Workitem *, Workitem *> > Deletable_Sublists;

      DEPSPAWN_PROFILEDEFINITION(const auto t0 = std::chrono::high_resolution_clock::now());
      //DEPSPAWN_PROFILEDEFINITION(unsigned int profile_deleted_workitems = 0);
      DEPSPAWN_PROFILEACTION(profile_erases++);

      Workitem *lastkeep = worklist;
      Workitem *last_workitem, *p;
      
      unsigned int deletable_workitems = 0;

      for(p = lastkeep->next; p != nullptr; p = p->next) {
        
        if( p->status != Status_t::Deallocatable ) {
          
          if(p->status == Status_t::Done) {
            Dones.push_back(p);
          }
          
          if (deletable_workitems > 4) { //We ask for a minimum that justifies the cost
            Deletable_Sublists.emplace_back(lastkeep->next, last_workitem); // Deleted sublist
            lastkeep->next = p;
            //DEPSPAWN_PROFILEACTION(profile_deleted_workitems += deletable_workitems);
          }
          
          lastkeep = p;
          deletable_workitems = 0;
          
        } else {
          deletable_workitems++;
        }
        
        last_workitem = p;
      }

      if (lastkeep->next != nullptr) {
        Deletable_Sublists.emplace_back(lastkeep->next, last_workitem);
        lastkeep->next = nullptr;
        //DEPSPAWN_PROFILEACTION(profile_deleted_workitems += deletable_workitems);
      }

      std::atomic_thread_fence(std::memory_order_seq_cst);

      //DEPSPAWN_PROFILEACTION(printf("D %u (%u) (%u) %c\n", profile_deleted_workitems, (unsigned)Dones.size(), (unsigned)Deletable_Sublists.size(), ));

      for(p = worklist.load(); p != worklist_wait_hint; p = p->next) {
        
        while(p->status == Status_t::Filling) { } // Waits until work p has its dependencies
        
        if(! (p->optFlags_ & (OptFlags::PendingFills|OptFlags::FatherScape)) ) {
          break;
        }
        
      }

      for (int i = 0; i < Dones.size(); i++) {
        while (Dones[i]->status == Status_t::Done) { }
      }
      Dones.clear(); //Needed because it is static!

      for (int i = 0; i < Deletable_Sublists.size(); i++) {
        Workitem * const begin = Deletable_Sublists[i].first;
        Workitem * const end = Deletable_Sublists[i].second;
        DEPSPAWN_DEBUGACTION(
                             for(Workitem *q = begin;  q != end->next; q = q->next) {
                               assert(q->args == nullptr);
                               assert(q->status == Status_t::Deallocatable);
                               if (q->deps) {
                                 printf("%p -> %p\n", q, q->deps);
                                 assert(q->deps == nullptr);
                               }
                             }
                             ); // END DEPSPAWN_DEBUGACTION
        
        Workitem::Pool.freeLinkedList(begin, end);
      }
      Deletable_Sublists.clear(); //Needed because it is static!
      
      DEPSPAWN_PROFILEACTION(profile_time_eraser_waiting += std::chrono::duration <double>(std::chrono::high_resolution_clock::now() - t0).count());
      
      /*
       for(p = next; p; p = p->next) {
         last_workitem = p;
         if( p->status != Status_t::Deallocatable )
         lastkeep = p;
       }
       
       Workitem *dp = lastkeep->next; // Everything from here will be deleted
       lastkeep->next = nullptr;
       
       if (dp != nullptr) {
       
         for(p = worklist; p != worklist_wait_hint; p = p->next) {
           while(p->status == Status_t::Filling) { } // Waits until work p has its dependencies
           if(! (p->optFlags_ & (OptFlags::PendingFills|OptFlags::FatherScape)) ) {
             break;
           }
         }
       
         DEPSPAWN_PROFILEACTION(profile_time_eraser_waiting += std::chrono::duration <double>(std::chrono::high_resolution_clock::now() - t0).count());
       
         DEPSPAWN_DEBUGACTION(
           for(p = dp; p; p = p->next) {
             assert(p-> args == nullptr);
             if (p->deps) {
               printf("%p -> %p\n", p, p->deps);
               assert(p->deps == nullptr);
             }
           }
         ); // END DEPSPAWN_DEBUGACTION
       
         Workitem::Pool.freeLinkedList(dp, last_workitem);
       }
       */
      
      eraser_assigned = false;
    }
    
  } //namespace internal
  
} //namespace depspawn
