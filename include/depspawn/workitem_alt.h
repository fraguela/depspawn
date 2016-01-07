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
/// \file     workitem_alt.h
/// \brief    Definition and API of an alternative implementation of Workitem
/// \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#ifndef __WORKITEM_ALT_H
#define __WORKITEM_ALT_H

#include <tbb/spin_mutex.h>


namespace depspawn {
  
  namespace internal {
    
    /// Status of a Workitem
    enum status_t { Filling = 0, Waiting = 1, Ready = 2, Ready2 = 3, Running = 4, Done = 5, Deallocatable = 6};
    
    /// Internal representation of the work associated to a spawn
    struct Workitem {
      arg_info *args;                   ///< List of parameters, ordered by memory position
      tbb::atomic<status_t> status;     ///< State of this Workitem
      tbb::atomic<unsigned int> curw;   ///< Workitem unique identifier
      Workitem* volatile next;          ///< next Workitem in the worklist
      volatile unsigned int idw;        ///< Identifier of the Workitem currently being analyzed during the insertion in the worklist
      tbb::atomic<int> ndependencies;   ///< Number of works on which the current task depends
      /// Depency on this Workitem
      struct _dep {
        Workitem* w;                    ///< Workitem that depends on the current task
        _dep* next;                     ///< Next dependency link
        /// Class pool
        static Pool_t<_dep, DEPSPAWN_SCALABLE_POOL> Pool;
        //static void* operator new(size_t isize) { return scalable_malloc(isize); };
        //static void operator delete(void* p) { scalable_free(p); };
      } * volatile deps,                ///< Head of the list of dependencies on this Workitem
        * volatile lastdep;             ///< Tail of the list of dependencies on this Workitem
      AbstractRunner* task;             ///< Task that encapsulates the work associated to this Workitem
      tbb::atomic<int> nchildren;       ///< Number of children tasks + 1
      Workitem* father;                 ///< Workitem that spawned this Workitem
      tbb::spin_mutex deps_mutex;       ///< Mutex for critical section for insertion in the list of dependencies
      tbb::atomic<char> guard_;         ///< Critical for the correct control of steals
      
      /// Default constructor, for pool purposes
      Workitem() {}
      
      /// Build Workitem associated to a list of information on arguments
      Workitem(arg_info *iargs);

      /// Initialize empty/recycled Workitem with a list of information on arguments
      void init(arg_info *iargs);
      
      /// Provide task with work for this Workitem and insert it in the worklist
      void insert_in_worklist(AbstractRunner* itask);
      
      /// Mark the Workitem as completed and clean up as appropriate
      void finish_execution();
      
      /// \brief Launch this Workitem, which was waiting due to depencendes, for execution
      ///
      /// Several threads could try it simultaneously. A compare_and_swap on the status
      /// solves the problem.
      void post() {
        if(status.compare_and_swap(Ready2, Waiting) == Waiting)
          master_task->spawn((tbb::task&)*task);
      }
      
      /// Steal the work of this ready Workitem or return nullptr if unsuccessful
      AbstractBoxedFunction * steal();

      /// Class pool
      static Pool_t<Workitem, DEPSPAWN_SCALABLE_POOL> Pool;
      
    private:
      
      void free_deps();
      
    };
    
  } //namespace  internal
  
} //namespace depspawn

#endif // __WORKITEM_ALT_H
