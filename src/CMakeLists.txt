
#DepSpawn: Data Dependent Spawn library
#Copyright (C) 2012-2019 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# \file     CMakeLists.txt
# \author   Carlos H. Gonzalez  <cgonzalezv@udc.es>
# \author   Basilio B. Fraguela <basilio.fraguela@udc.es>

cmake_minimum_required( VERSION 2.8.7 )

#TODO: Could be protected with an #ifndef... #define... #endif
# add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/include/depspawn/fixed_defines.h
#                    COMMAND ${CMAKE_COMMAND} -E echo setting ${DEPSPAWN_DUMMY_POOL_STR}
# 		   COMMAND ${CMAKE_COMMAND} -E echo setting ${DEPSPAWN_SCALABLE_POOL_STR}
#                    COMMAND ${CMAKE_COMMAND} -E echo ${DEPSPAWN_DUMMY_POOL_STR} > fixed_defines.h
# 		   COMMAND ${CMAKE_COMMAND} -E echo ${DEPSPAWN_SCALABLE_POOL_STR} >> fixed_defines.h
# 		   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/include/depspawn
# 		   COMMENT "Fixing macros that should be consistent in the compilation of the library and the client codes")

configure_file( ${PROJECT_SOURCE_DIR}/include/depspawn/fixed_defines.h.in ${PROJECT_SOURCE_DIR}/include/depspawn/fixed_defines.h )

#set_source_files_properties( ${PROJECT_SOURCE_DIR}/include/depspawn/fixed_defines.h PROPERTIES GENERATED TRUE)

add_library( depspawn STATIC depspawn.cpp ${PROJECT_SOURCE_DIR}/include/depspawn/fixed_defines.h )


#install

install( TARGETS depspawn EXPORT depspawn-targets
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib
         INCLUDES DESTINATION include )

install( DIRECTORY ../include/depspawn
         DESTINATION include
         FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" )

# downstream install

set( ConfigPackageLocation lib/cmake/depspawn )

configure_file( depspawnConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/depspawnConfig.cmake @ONLY )

install( EXPORT depspawn-targets DESTINATION ${ConfigPackageLocation} FILE depspawn-targets.cmake )

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/depspawnConfig.cmake DESTINATION ${ConfigPackageLocation} )

