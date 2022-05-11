
#DepSpawn: Data Dependent Spawn library
#Copyright (C) 2012-2022 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
#
# Distributed under the MIT License. (See accompanying file LICENSE)

# \file     CMakeLists.txt
# \brief    CMakeLists.txt for the tests of the project
# \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
# \author   Basilio B. Fraguela <basilio.fraguela@udc.es>

add_definitions( -DBZ_THREADSAFE -DBZ_THREADSAFE_USE_TBB )

include_directories( ${PROJECT_SOURCE_DIR}/src )
link_libraries( depspawn )

if(DEPSPAWN_USE_TBB)
	link_libraries( tbb tbbmalloc )
endif()

link_libraries( pthread )

if(${UNIX})
  if(${APPLE})
    #The reason is that in OSX El Capitan (10.11) we need cmake -E env
    cmake_minimum_required( VERSION 3.1.3 )
    set( CMAKE_MACOSX_RPATH ON )
  else(${APPLE})
    link_libraries( rt )
  endif(${APPLE})
endif(${UNIX})

if (NOT DEPSPAWN_BLITZ )
  set(DEPSPAWN_BLITZ No CACHE STRING
      "Options are: No, Old, New"
      FORCE)
endif()
string( TOLOWER "${DEPSPAWN_BLITZ}" depspawn_blitz )
if(depspawn_blitz STREQUAL "new")
	include(ExternalProject)
	ExternalProject_Add(depspawn_blitz 
	                    SOURCE_DIR ${CMAKE_SOURCE_DIR}/depspawn-blitz-1.0.2
					    INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
						CMAKE_ARGS -D CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
					   )
	include_directories( ${CMAKE_INSTALL_PREFIX}/include )
elseif(depspawn_blitz STREQUAL "old")
	include_directories( ${PROJECT_SOURCE_DIR}/depspawn-blitz-0.10 )
endif()

set( TESTBINARIES super_simple_test mxv_basic test_dep_for_basicchild_stdfunc test_dep_for_basicchild_boostfunc test_by_value test_lambda test_subobject test_wait_children_dep test_out_scope test_donot_wait_for_ancestors test_int test_array test_wait_for test_Observer test_depspawn_sync histogram )

set( BLITZTESTBINARIES mxm mxv test_blitz_array testreduction1_fix1 )

set( NOTESTBINARIES mxv_basic_orix test_ignore test_array_sizes bench_spawn bench_sched_perf fib )

set( BLITZNOTESTBINARIES testreduction1_orix )

foreach(b ${TESTBINARIES} ${NOTESTBINARIES}) 
  add_executable( ${b} ${b}.cpp )
endforeach(b)

if(NOT depspawn_blitz STREQUAL "no")
	foreach(b ${BLITZTESTBINARIES} ${BLITZNOTESTBINARIES} )
	  add_executable( ${b} ${b}.cpp )
	endforeach(b)
	list(APPEND TESTBINARIES ${BLITZTESTBINARIES})
endif()

add_custom_target(check)
foreach(b ${TESTBINARIES} )
  
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

