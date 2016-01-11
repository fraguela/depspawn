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