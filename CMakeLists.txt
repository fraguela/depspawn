
#DepSpawn: Data Dependent Spawn library
#Copyright (C) 2012-2022 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
#
# Distributed under the MIT License. (See accompanying file LICENSE)

# \file     CMakeLists.txt
# \brief    Main CMakeLists.txt of the project
# \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
# \author   Basilio B. Fraguela <basilio.fraguela@udc.es>

project( DepSpawn )

cmake_minimum_required( VERSION 3.1.3 )

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

find_package( Boost )

if(${Boost_FOUND})
  include_directories( ${Boost_INCLUDE_DIRS} )
else(${Boost_FOUND})
  message( WARNING "***********************************************\n           Boost could not be found            \nDepSpawn will use and install a subset of boost\n***********************************************")
  install( DIRECTORY ./include/boost
           DESTINATION include )
endif(${Boost_FOUND})

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

option( DEPSPAWN_USE_TBB       "Rely on TBB" OFF)
option( DEPSPAWN_FAST_START    "Launcher thread executes tasks when many are enqueued" ON)
option( DEPSPAWN_DUMMY_POOL    "Do not actually use object pools" OFF)
option( DEPSPAWN_SCALABLE_POOL "Use tbb::scalable_malloc/free instead of std::malloc/free" OFF)
option( DEPSPAWN_PROFILE       "Do profiling" OFF)

if(DEPSPAWN_FAST_START)
# Only affects .cpp files, so no need to fix in headers
  add_definitions(-DDEPSPAWN_FAST_START)
endif(DEPSPAWN_FAST_START)

# if(DEPSPAWN_DUMMY_POOL)
#   #Do not use add_definitions because it is already obtained from fixed_defines.h
#   set( DEPSPAWN_DUMMY_POOL_STR " \#define DEPSPAWN_DUMMY_POOL 1")
# else(DEPSPAWN_DUMMY_POOL)
#   set( DEPSPAWN_DUMMY_POOL_STR " \#undef DEPSPAWN_DUMMY_POOL")
# endif(DEPSPAWN_DUMMY_POOL)

if(DEPSPAWN_SCALABLE_POOL)
# Do not use add_definitions because it is already obtained from fixed_defines.h
  set( DEPSPAWN_SCALABLE_POOL_STR "true")
else(DEPSPAWN_SCALABLE_POOL)
  set( DEPSPAWN_SCALABLE_POOL_STR "false" )
endif(DEPSPAWN_SCALABLE_POOL)

if(DEPSPAWN_PROFILE)
# Only affects .cpp files, so no need to fix in headers
  add_definitions(-DDEPSPAWN_PROFILE)
endif(DEPSPAWN_PROFILE)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )

include_directories( include )

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
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/doc
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/doc ${CMAKE_CURRENT_BINARY_DIR}/doc
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/doc
    MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/doc/Doxyfile
    DEPENDS ${DOXY_DEPS}
    COMMENT "Generating HTML documentation"
  )
	
  add_custom_target(doc ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/doc/html/index.html)
  
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/html/ DESTINATION share/depspawn/doc)

endif(DOXYGEN_FOUND)
