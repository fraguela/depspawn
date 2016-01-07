
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
# \brief    Main CMakeLists.txt of the project
# \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
# \author   Basilio B. Fraguela <basilio.fraguela@udc.es>

cmake_minimum_required( VERSION 2.8 )

project( DepSpawn )

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

find_package( Boost REQUIRED )

#message( "Boost Found : " ${Boost_FOUND})
#message( "InclDir : " ${Boost_INCLUDE_DIR})
#message( "LibDir  : " ${Boost_LIBRARY_DIR})
#message( "InclDirs : " ${Boost_INCLUDE_DIRS})
#message( "LibDirs  : " ${Boost_LIBRARY_DIRS})
#message( "Libraries: " ${Boost_LIBRARIES})
#message( "Version  : " ${Boost_VERSION})
#message( "V        : " ${Boost_MAJOR_VERSION} ${Boost_MINOR_VERSION} ${Boost_SUBMINOR_VERSION})
#
#message( "CMAKE_BINARY_DIR : " ${CMAKE_BINARY_DIR} )
#message( "CMAKE_SOURCE_DIR : " ${CMAKE_SOURCE_DIR} )

set( DEPSPAWN_FAST_START     ON  CACHE BOOL "Launcher thread executes tasks when many are enqueued")
set( DEPSPAWN_DUMMY_POOL     ON  CACHE BOOL "Do not actually use a poll")
set( DEPSPAWN_SCALABLE_POOL  OFF CACHE BOOL "Use tbb::scalable_malloc/free instead of std::malloc/free")
set( DEPSPAWN_PROFILE        OFF CACHE BOOL "Do profiling")

if(DEPSPAWN_FAST_START)
# Only affects .cpp files, so no need to fix in headers
  add_definitions(-DDEPSPAWN_FAST_START)
endif(DEPSPAWN_FAST_START)

if(DEPSPAWN_DUMMY_POOL)
  #Do not use add_definitions because it is already obtained from fixed_defines.h
  set( DEPSPAWN_DUMMY_POOL_STR "\\#define DEPSPAWN_DUMMY_POOL 1")
else(DEPSPAWN_DUMMY_POOL)
  set( DEPSPAWN_DUMMY_POOL_STR "\\#undef DEPSPAWN_DUMMY_POOL")
endif(DEPSPAWN_DUMMY_POOL)

if(DEPSPAWN_SCALABLE_POOL)
  #Do not use add_definitions because it is already obtained from fixed_defines.h
  set( DEPSPAWN_SCALABLE_POOL_STR "\\#define DEPSPAWN_SCALABLE_POOL true")
else(DEPSPAWN_SCALABLE_POOL)
  set( DEPSPAWN_SCALABLE_POOL_STR "\\#define DEPSPAWN_SCALABLE_POOL false")
endif(DEPSPAWN_SCALABLE_POOL)

if(DEPSPAWN_PROFILE)
# Only affects .cpp files, so no need to fix in headers
  add_definitions(-DDEPSPAWN_PROFILE)
endif(DEPSPAWN_PROFILE)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x" )

include_directories( ${Boost_INCLUDE_DIR} include )

add_subdirectory( src )
add_subdirectory( tests )

find_package(Doxygen)
if(DOXYGEN_FOUND)
  ##configure_file(Doxyfile.in Doxyfile @ONLY) ${CMAKE_CURRENT_BINARY_DIR}
  
  set(DOXY_DEPS
    ${PROJECT_SOURCE_DIR}/doc/main.dox
    ${PROJECT_SOURCE_DIR}/src/depspawn.cpp
	${PROJECT_SOURCE_DIR}/src/workitem.cpp
	${PROJECT_SOURCE_DIR}/include/depspawn/depspawn.h
    ${PROJECT_SOURCE_DIR}/include/depspawn/workitem.h
    ${PROJECT_SOURCE_DIR}/include/depspawn/depspawn_utils.h
	${PROJECT_SOURCE_DIR}/include/depspawn/LinkedListPool.h
	${PROJECT_SOURCE_DIR}/include/depspawn/DummyLinkedListPool.h
  )
  
  add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/doc/html/index.html
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
	COMMAND cmake -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/doc
	COMMAND cmake -E copy_directory ${PROJECT_SOURCE_DIR}/doc ${CMAKE_CURRENT_BINARY_DIR}/doc
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/doc
    MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/doc/Doxyfile
    DEPENDS ${DOXY_DEPS}
    COMMENT "Generating HTML documentation"
   	)
	
  add_custom_target(doc ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/doc/html/index.html)
  
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/html/ DESTINATION share/depspawn/doc)

endif(DOXYGEN_FOUND)
