
#DepSpawn: Data Dependent Spawn library
#Copyright (C) 2012-2021 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
#
# Distributed under the MIT License. (See accompanying file LICENSE)

# \file     CMakeLists.txt
# \brief    CMakeLists.txt for the tests of the project
# \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
# \author   Basilio B. Fraguela <basilio.fraguela@udc.es>

cmake_minimum_required( VERSION 2.8.7 )

add_definitions( -DBZ_THREADSAFE -DBZ_THREADSAFE_USE_TBB )

include_directories( ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/depspawn-blitz-0.10 )
link_libraries( depspawn tbb tbbmalloc )

if(${UNIX})
  if(${APPLE})
    #The reason is that in OSX El Capitan (10.11) we need cmake -E env
    cmake_minimum_required( VERSION 3.1.3 )
    set( CMAKE_MACOSX_RPATH ON )
  else(${APPLE})
    link_libraries( rt )
  endif(${APPLE})
endif(${UNIX})

set( NOBLITZTESTBINARIES super_simple_test mxv_basic test_dep_for_basicchild_stdfunc test_dep_for_basicchild_boostfunc test_by_value test_lambda test_subobject test_wait_children_dep test_out_scope test_donot_wait_for_ancestors test_int test_array test_wait_for test_Observer test_depspawn_sync histogram )

set( BLITZTESTBINARIES mxm mxv test_blitz_array testreduction1_fix1 )

set( NOTESTBINARIES testreduction1_orix mxv_basic_orix test_ignore test_array_sizes bench_spawn bench_sched_perf fib )


foreach(b ${NOBLITZTESTBINARIES} ${BLITZTESTBINARIES} ${NOTESTBINARIES} )
  add_executable( ${b} ${b}.cpp )
endforeach(b)

add_custom_target(check)
foreach(b ${NOBLITZTESTBINARIES} ${BLITZTESTBINARIES} )
  
  add_custom_command(TARGET check PRE_BUILD COMMAND ${CMAKE_COMMAND} -E echo test ${b} :)
  if(${APPLE})                   
    add_custom_command(TARGET check PRE_BUILD 
                      COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=$ENV{LIBRARY_PATH}:$ENV{LD_LIBRARY_PATH} ${b} || exit 1)
  else(${APPLE})
    add_custom_command(TARGET check PRE_BUILD COMMAND ${b} || exit 1)
  endif(${APPLE})
  add_custom_command(TARGET check PRE_BUILD COMMAND ${CMAKE_COMMAND} -E echo ------------------------)

endforeach(b)
add_custom_command(TARGET check PRE_BUILD COMMAND echo "*" TESTS DONE. SUCCESSFUL)
