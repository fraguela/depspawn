## Installation</p>


These instructions can also be found in the library [documentation](http://fraguela.github.io/depspawn/page_installation.html). 

### Requirements


 * A C++ compiler that supports C++11
 * [Intel® Threading Building Blocks (TBB)](http://threadingbuildingblocks.org) v3.0 update 6 or above (v4.3 update 6 for Mac OS X) 
 * [Boost](http://www.boost.org) v1.48 or above
 * [CMake](https://cmake.org) 2.8.7 or above (3.1.3 for Mac OS X)
 * Optionally: [Doxygen](http://www.doxygen.org) for building its documentation

### Setting up Intel TBB </p>


Intel TBB must be configured before building or using DepSpawn. This requires setting up some environment variables both to be compiled and to be used by an application during its execution. This usually requires executing the script 
 
 - `${TBBROOT}/bin/tbbvars.sh` if your shell is `bash`/`ksh`, or
 - `${TBBROOT}/bin/tbbvars.csh` if your shell is `csh`

 where `${TBBROOT}` is the directory where TBB has been installed. The file includes the definition of the variable `TBBROOT`, which must point to the directory where TBB has been installed, but it is usually left empty during the installation. For this reason right after intalling TBB you must edit this file to define this variable. In order to do this, open the file and look for a line like
 
      export TBBROOT="SUBSTITUTE_INSTALL_DIR_HERE"

 in the case of `tbbvars.sh` or 

      setenv TBBROOT "SUBSTITUTE_INSTALL_DIR_HERE"
	  
 in the case of `tbbvars.csh` and put the root directory for your TBB installation between the double quotes.. 


### Step by step procedure 

1. Make sure TBBs are automatically found by your compiler following the steps described above.

2. Unpack the DepSpawn tarball (e.g. `depspawn.tar.gz`)

		tar -xzf depspawn.tar.gz
		cd depspawn		
 or clone the project from its repository
 
	  git clone git@github.com:fraguela/depspawn.git
	
3. Create the temporary directory where the project will be built and enter it :

		mkdir build && cd build

4. Generate the files for building DepSpawn in the format that you prefer (Visual Studio projects, nmake makefiles, UNIX makefiles, Mac Xcode projects, ... ) using cmake.

    In this process you can use a graphical user interface for cmake such as `cmake-gui` in Unix/Mac OS X or `CMake-gui` in Windows, or a command-line interface such as `ccmake`. The process is explained here assuming this last possibility, as graphical user interfaces are not always available.
 
5. run `ccmake ..`
	
	 This will generate the files for building DepSpawn with the tool that cmake choses by default for your platform. Flag `-G` can be used to specify the kind of tool that you want to use. For example if you want to use Unix makefiles but they are not the default in you system, run <tt>ccmake -G 'Unix Makefiles' ..</tt>

	 Run `ccmake --help` for additional options and details.

6. Press letter `c` to configure your build.
7. Provide the values you wish for the variables that appear in the screen. The most relevant ones are:
	- `CMAKE_BUILD_TYPE` : String that specifies the build type. Its possible values are empty, Debug, Release, RelWithDebInfo and MinSizeRel.
	- `CMAKE_INSTALL_PREFIX` : Directory where DepSpawn will be installed
		
	 The variables that begin with `DEPSPAWN` change internal behaviors in the runtime of the library. Thus in principle they are of interest only to the developers of the library, although users can play with them to see how they impact in their applications. They are:
	- `DEPSPAWN_DUMMY_POOL` : Boolean that if `ON` turns off the usage of pools by the library to manage the memory of its internal objects.
    - `DEPSPAWN_FAST_START` : If this boolean is `ON`, when a thread that is spawning tasks detects that there are too many ready pending tasks waiting to be executed, it stops and executes one of them before resuming its current task.
    - `DEPSPAWN_PROFILE` : When this boolean is `ON` the library gathers statistics about its internal operations.
    - `DEPSPAWN_SCALABLE_POOL` : This boolean controls whether the library managed its memory by means of the `tbbmalloc` library (if `ON`) or the standard C++ library (if `OFF`).
8. When you are done, press `c` to re-configure cmake with the new values.
9. Press `g` to generate the files that will be used to build DepSpawn and exit cmake.
10. The rest of this explanation assumes that UNIX makefiles were generated in the previous step. 

	Run `make` in order to build the library and its tests.  The degree of optimization, debugging information and assertions enabled depends on the value you chose for variable `CMAKE_BUILD_TYPE`.
	
    You can use the flag `-j` to speedup the building process. For example, `make -j4` will use 4 parallel processes, while `make -j` will use one parallel process per available hardware thread.
11. (Optionally) run `make check` in order to run the DepSpawn tests. 
12. Run `make install` 

    This installs DepSpawn under the directory you specified for the `CMAKE_INSTALL_PREFIX` variable. If you left it empty, the default base directories will be `/usr/local` in Unix and `c:/Program Files` in Windows. 

    The installation places
	- The depspawn library in the subdirectory `lib`
	- The header files in the subdirectory `include/depspawn`
	- If [Doxygen](http://www.doxygen.org) is present, a manual in `share/depspawn`

13. (Optional *but stronly advised*) Depspawn provides a slightly modified version of the [Blitz++ library](http://www.sourceforge.net/projects/blitz) that allows to use Blitz++ arrays and subarrays as arguments of the spawned functions, thus providing a very nice notation for array-based computations. The installation of this library, located in the directory `depspawn/depspawn-blitz-0.10`, is based on the traditional `configure` and `make` mechanisms. The installation instructions and Blitz++ documentation are found in the library directory.

14. You can remove the `depspawn` directory generated by the unpacking of the tarball or the cloning of the project repository.
