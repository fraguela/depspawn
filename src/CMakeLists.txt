
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

#TODO: Could be protected with an #ifndef... #define... #endif
add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/include/depspawn/fixed_defines.h
                   COMMAND ${CMAKE_COMMAND} -E echo setting ${DEPSPAWN_DUMMY_POOL_STR}
				   COMMAND ${CMAKE_COMMAND} -E echo setting ${DEPSPAWN_SCALABLE_POOL_STR}
                   COMMAND ${CMAKE_COMMAND} -E echo ${DEPSPAWN_DUMMY_POOL_STR} > fixed_defines.h
				   COMMAND ${CMAKE_COMMAND} -E echo ${DEPSPAWN_SCALABLE_POOL_STR} >> fixed_defines.h
				   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/include/depspawn
				   COMMENT "Fixing macros that should be consistent in the compilation of the library and the client codes")

set_source_files_properties( ${PROJECT_SOURCE_DIR}/include/depspawn/fixed_defines.h PROPERTIES GENERATED TRUE)

add_library( depspawn STATIC depspawn.cpp ${PROJECT_SOURCE_DIR}/include/depspawn/fixed_defines.h )


#install

install( TARGETS depspawn
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib)

install( DIRECTORY ../include/depspawn
         DESTINATION include
         FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" )
