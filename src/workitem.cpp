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
/// \file     workitem.cpp
/// \brief    Implementation of Workitem
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

namespace depspawn {
  
  namespace internal {
    
    Workitem::Workitem(arg_info *iargs, int nargs) :
    args(iargs), status(Filling), task(nullptr),
    father(enum_thr_spec_father.local()), next(nullptr),
    deps(nullptr), lastdep(nullptr), nargs_(static_cast<char>(nargs))
    {
      ndependencies = 0;
      nchildren = 1;
      guard_ = 0;
      if(father)
        father->nchildren.fetch_and_increment();
    }
    
    void Workitem::init(arg_info *iargs, int nargs)
    {
      args = iargs;
      status = Filling;
      task = nullptr;
      father = enum_thr_spec_father.local();
      next = nullptr;
      deps = lastdep = nullptr;
      ndependencies = 0;
      nchildren = 1;
      guard_ = 0;
      nargs_ = nargs;
      if(father)
        father->nchildren.fetch_and_increment();
    }
    
    AbstractBoxedFunction * Workitem::steal() {
      AbstractBoxedFunction * ret = nullptr;
      if( (status == Ready) && (guard_.compare_and_swap(1, 0) == 0) ) {
        ret = task->steal();
        guard_ = 2;
      }
      return ret;
    }

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
      
      DEPSPAWN_PROFILEACTION(profile_jobs++);
      
#ifdef DEPSPAWN_FAST_START
      AbstractBoxedFunction *stolen_abf = nullptr;
      Workitem* fast_arr[FAST_ARR_SZ];
      int nready = 0;
#endif
      
      task = itask;
      
      Workitem* ancestor = father;
      
      do {
        p = worklist;
        next = p;
      } while(worklist.compare_and_swap(this, p) != p);

      for(p = next; p != nullptr; p = p->next) {

        DEPSPAWN_PROFILEACTION(profile_workitems_in_list_lcl++);
        
        // sometimes p == this, even if it shouldn't
        if(p == this) { // fprintf(stderr, "myself?\n");
          continue; // FIXIT
        }
        if(p == ancestor) {
          ancestor = p->father;
          // TODO: is_contained should be adapted to use argv
          if ( (ancestor != nullptr) && is_contained(this, ancestor)) {
            break;
          }
        } else {
          const status_t tmp_status = p->status;
          if(tmp_status < Done) {
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
                
                tbb::mutex::scoped_lock lock(p->deps_mutex);
                if(!p->deps)
                  p->deps = p->lastdep = newdep;
                else {
                  p->lastdep->next = newdep;
                  p->lastdep = p->lastdep->next;
                }
                lock.release();
                DEPSPAWN_DEBUGACTION(
                                     /* You can be linking to Done's that are waiting for you to Fill-in
                                       but NOT for Deallocatable Workitems */
                                     if(p->status == Deallocatable) {
                                       printf("%p -> Deallocatable %p (was %d)", this, p, (int)tmp_status);
                                       assert(false);
                                     }
                                     ); // END DEPSPAWN_DEBUGACTION
                if (arg_p->wr &&
                    (arg_w->is_array() ? arg_w->is_contained_array(arg_p) : contains(arg_p, arg_w))) {
                  nargs--; //printf("%d %d %lu\n", nargs, arg_w_i, arg_w->addr);
                  if (!nargs) {
                    /* The optimal thing to do is to just make this goto to leave the main loop and insert
                     the Workitem waiting. But in tests with repeated spawns this leads to very fast insertion
                     that slows down the performance */
                    //goto OUT_MAIN_insert_in_worklist_LOOP;
                    argv[0] = nullptr;
                    break;
                  } else {
                    for (int i = arg_w_i; i <= nargs; i++) {
                      argv[i] = argv[i+1];
                    }
                  }
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
            if ( p->status == Ready ) {
              fast_arr[nready & (FAST_ARR_SZ - 1)] = p;
              nready++;
            }
#endif
          }
        }
      }
      
OUT_MAIN_insert_in_worklist_LOOP:
      
#ifdef DEPSPAWN_FAST_START
      if (nready > FAST_THRESHOLD) {
        DEPSPAWN_PROFILEACTION(profile_steal_attempts++);
        nready = (nready - 1) & (FAST_ARR_SZ - 1);
        do {
          p = fast_arr[nready--];
          stolen_abf = p->steal();
        } while ( (stolen_abf == nullptr) && (nready >= 0) );
      }
#endif
      
      // This is set after stealing work because this way
      // the fast_arr workitems should not have been deallocated
      //status = (!ndependencies) ? Ready : Waiting;
      if (!ndependencies) {
        status = Ready;
        master_task->spawn(*task);
      } else {
        status = Waiting;
      }

#ifdef DEPSPAWN_FAST_START
      if (stolen_abf != nullptr) {
        DEPSPAWN_PROFILEACTION(profile_steals++);
        stolen_abf->run_in_env();
        delete stolen_abf;
      }
#endif
      
      DEPSPAWN_PROFILEACTION(profile_workitems_in_list += profile_workitems_in_list_lcl);
      DEPSPAWN_PROFILEACTION(profile_workitems_in_list_active += profile_workitems_in_list_active_lcl);
    }
    
    void Workitem::finish_execution()
    { Workitem *p, *ph, *lastkeep;
      
      if(nchildren.fetch_and_decrement() != 1)
        return;
      
      bool erase = false;
      Workitem *current = this;

      do {

        // Finish work
        current->status = Done;
      
        /* Without this fence the change of status can be unseen by Workitems that are Filling in */
        tbb::atomic_fence();
      
        erase = erase || ((((((intptr_t)current)>>8)&0xfff) < 32) && !ObserversAtWork && !eraser_assigned && !eraser_assigned.compare_and_swap(true, false));
      
        ph = worklist;
        lastkeep = ph;
        for(p = ph; p && p != this; p = p->next) { //wait until this, not current
          while(p->status == Filling) {}
          if(p->status != Deallocatable)
            lastkeep = p;
        }
      
        // free lists
        //delete ctx_->task;
      
        Workitem::_dep *idep = current->deps;
        if (idep != nullptr) {
          do {
            if(idep->w->ndependencies.fetch_and_decrement() == 1) {
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
      
        current->status = Deallocatable;
      
        current = p;

      } while ((p != nullptr) && (p->nchildren.fetch_and_decrement() == 1));
      
      if(erase) {
        DEPSPAWN_PROFILEDEFINITION(const tbb::tick_count t0 = tbb::tick_count::now());
        DEPSPAWN_PROFILEACTION(profile_erases++;);

        Workitem *last_workitem = this;
        for(p = next; p; p = p->next) {
          last_workitem = p;
          if(p->status != Deallocatable)
            lastkeep = p;
        }
        
        Workitem *dp = lastkeep->next; // Everything from here will be deleted
        lastkeep->next = nullptr;
        
        if (dp != nullptr) {

          for(p = worklist; p != ph; p = p->next) {
            while(p->status == Filling) { } // Waits until work p has its dependencies
          }
          
          DEPSPAWN_PROFILEACTION(profile_time_eraser_waiting += (tbb::tick_count::now() - t0).seconds());

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
        
        
        eraser_assigned = false;
      }
      
    }
    
  } //namespace internal
  
} //namespace depspawn

