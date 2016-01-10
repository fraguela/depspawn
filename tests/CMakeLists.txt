
#DepSpawn: Data Dependent Spawn library
#Copyright (C) 2012-2016 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
#
#This file is part of DepSpawn.
#
#DepSpawn is free software; you can redistribute it and/or modify it under the terms
#of the GNU General Public License as published by the Free Software Foundation;
#either version 2 as published by the Free Software Foundation.
#
#DepSpawn is distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
#WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
#PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with Threading Building Blocks; if not, write to the Free Software
#Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
#As a special exception, you may use this file as part of a free software
#library without restriction.  Specifically, if other files instantiate
#templates or use macros or inline functions from this file, or you compile
#this file and link it with other files to produce an executable, this
#file does not by itself cause the resulting executable to be covered by
#the GNU General Public License.  This exception does not however
#invalidate any other reasons why the executable file might be covered by
#the GNU General Public License.

# \file     CMakeLists.txt
# \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
# \author   Basilio B. Fraguela <basilio.fraguela@udc.es>

cmake_minimum_required( VERSION 2.8.7 )

add_definitions( -DBZ_THREADSAFE -DBZ_THREADSAFE_USE_TBB )

include_directories( ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/depspawn-blitz-0.10 )
link_libraries( depspawn tbb tbbmalloc )

set( NOBLITZTESTBINARIES super_simple_test mxv_basic test_dep_for_basicchild_stdfunc test_dep_for_basicchild_boostfunc test_by_value test_lambda test_subobject test_wait_children_dep test_out_scope test_donot_wait_for_ancestors test_int test_array test_wait_for test_Observer test_depspawn_sync histogram )

set( BLITZTESTBINARIES mxm mxv test_blitz_array testreduction1_fix1 )

set( NOTESTBINARIES testreduction1_orix mxv_basic_orix test_ignore test_array_sizes bench_spawn )


foreach(b ${NOBLITZTESTBINARIES} ${BLITZTESTBINARIES} ${NOTESTBINARIES} )
  add_executable( ${b} ${b}.cpp )
endforeach(b)

add_custom_target(check)
foreach(b ${NOBLITZTESTBINARIES} ${BLITZTESTBINARIES} )
  add_custom_command(TARGET check PRE_BUILD
                     COMMAND echo test ${b} : -------------
                                         COMMAND ${b} || exit 1
                                         COMMAND echo ------------------------
                    )
endforeach(b)
add_custom_command(TARGET check PRE_BUILD COMMAND echo "*" TESTS DONE. SUCCESSFUL)
