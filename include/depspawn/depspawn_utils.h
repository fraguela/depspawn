/*
 DepSpawn: Data Dependent Spawn library
 Copyright (C) 2012-2019 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

///
/// \file     depspawn_utils.h
/// \brief    Some helper macros
/// \author   Basilio B. Fraguela <basilio.fraguela@udc.es>
///

#ifndef __DEPSPAWNUTILS_H
#define __DEPSPAWNUTILS_H



#ifdef DEPSPAWN_PROFILE

/// Code executed only for profiling purposes
#define DEPSPAWN_PROFILEACTION(...) do{ __VA_ARGS__ ; }while(0)

/// Definition used only for profiling purposes
#define DEPSPAWN_PROFILEDEFINITION(...) __VA_ARGS__

#else

#define DEPSPAWN_PROFILEACTION(...) /* no profiling */
#define DEPSPAWN_PROFILEDEFINITION(...) /* no profiling */

#endif // DEPSPAWN_PROFILE



#ifndef NDEBUG

/// Code executed only for debugging purposes
#define DEPSPAWN_DEBUGACTION(...) do{ __VA_ARGS__ ; }while(0)

/// Definition used only for debugging purposes
#define DEPSPAWN_DEBUGDEFINITION(...) __VA_ARGS__

#else

#define DEPSPAWN_DEBUGACTION(...) /* no debugging */
#define DEPSPAWN_DEBUGDEFINITION(...) /* no debugging */

#endif // NDEBUG



#endif // __DEPSPAWNUTILS_H
