## Data Dependent Spawn (DepSpawn) </p>

A library for parallelizing applications by means of tasks that automatically respect their data dependencies.

Website: [http://depspawn.des.udc.es](http://depspawn.des.udc.es)

License: GPLv2 with the [runtime exception](https://www.threadingbuildingblocks.org/licensing#runtime-exception)

### Requirements


 * A C++ compiler that supports C++11
 * [Intel (R) Threading Building Blocks (TBB)](http://threadingbuildingblocks.org) v3.0 update 6 or above
 * [Boost](http://www.boost.org) v1.48 or above
 * [CMake](https://cmake.org) 2.8 or above
 * Optionally: [Doxygen](http://www.doxygen.org) for building its documentation

### Documentation

The best source for starting with DepSpawn is its [documentation](http://fraguela.github.io/depspawn/), which includes a step-by-step introduction that covers its semantics and API and discusses several interesting tips and tricks. A more detailed description of the DepSpawn programming model can be found the publication [A framework for argument-based task synchronization with automatic detection of dependencies](http://www.des.udc.es/~basilio/papers/Gonzalez13-DepSpawn.pdf) ([DOI 10.1016/j.parco.2013.04.012](http://dx.doi.org/10.1016/j.parco.2013.04.012)), although it lacks some of the new synchronization mechanisms added in version 1.0.


