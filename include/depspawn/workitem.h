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
/// \file     workitem.h
/// \brief    Definition and API of Workitem
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#ifndef __WORKITEM_H
#define __WORKITEM_H

namespace depspawn {
  
  namespace internal {
    
    /// Internal representation of the work associated to a spawn
    struct Workitem {
      
      /// Status of a Workitem
      struct Status_t {
        enum internal_Status_t : short int {
          Filling = 0,
          Waiting = 1,
          Ready = 2,
          Running = 3,
          Done = 4,
          Deallocatable = 5
        };
      };
      
      /// Bitmap for implementing optimizations
      struct OptFlags {
        enum internal_OptFlags : short int {
          PendingFills = 1,
          FatherScape  = 2
        };
      };
      
      volatile Status_t::internal_Status_t status;  ///< State of this Workitem
      short int optFlags_;
      tbb::atomic<char> deps_mutex_;    ///< Mutex for critical section for insertion in the list of dependencies
      tbb::atomic<char> guard_;         ///< Critical for the correct control of steals
      char nargs_;
      
      arg_info *args;                   ///< List of parameters, ordered by memory position
      Workitem* next;                   ///< next Workitem in the worklist
      Workitem* father;                 ///< Workitem that spawned this Workitem
      tbb::atomic<int> ndependencies;   ///< Number of works on which the current task depends
      tbb::atomic<int> nchildren;       ///< Number of children tasks + 1
      AbstractRunner* task;             ///< Task that encapsulates the work associated to this Workitem

      /// Depency on this Workitem
      struct _dep {
        Workitem* w;                    ///< Workitem that depends on the current task
        _dep* next;                     ///< Next dependency link
        /// Class pool
        static Pool_t<_dep, DEPSPAWN_SCALABLE_POOL> Pool;
        //static void* operator new(size_t isize) { return scalable_malloc(isize); };
        //static void operator delete(void* p) { scalable_free(p); };
      } *deps,                          ///< Head of the list of dependencies on this Workitem
        *lastdep;                       ///< Tail of the list of dependencies on this Workitem
      
      /// Default constructor, for pool purposes
      Workitem() {}
      
      /// Build Workitem associated to a list of information on arguments
      Workitem(arg_info *iargs, int nargs);
      
      /// Provide task with work for this Workitem and insert it in the worklist
      void insert_in_worklist(AbstractRunner* itask);
      
      /// Mark the Workitem as completed and clean up as appropriate
      void finish_execution();
      
      /// \brief Launch this Workitem, which was waiting due to depencendes, for execution
      ///
      /// Only one thread (the one that completed the task associated to the last dependency)
      /// calls this function, after an atomic decenment zeroes ndependencies
      void post() {
        status = Status_t::Ready;
        master_task->spawn((tbb::task&)*task);
      }
      
      /// Steal the work of this ready Workitem or return nullptr if unsuccessful
      AbstractBoxedFunction * steal();

      static void Clean_worklist(Workitem *worklist_wait_hint = nullptr);

      /// Class pool
      static Pool_t<Workitem, DEPSPAWN_SCALABLE_POOL> Pool;

    };
    
  } //namespace  internal
  
} //namespace depspawn

#endif // __WORKITEM_H
