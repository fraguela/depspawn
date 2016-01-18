## Data Dependent Spawn (DepSpawn) </p>

A library for parallelizing applications by turning functions into tasks that automatically respect their data dependencies.

All you need to do to execute in parallel a function `f(a, b, ...)` using DepSpawn is to rewrite the function call as `spawn(f, a, b, ...)`. Besides it will take care that the function is executed only when all its data dependencies are fulfilled. DepSpawn learns these dependencies automatically by analyzing the type of the function formal parameters following this strategy:

 - Arguments passed by value are necessarily read-only, that is, only inputs to the function, as it cannot modify the original argument provided.
 
 - Arguments passed by `const` reference are also only inputs, as, although they are passed by reference, the `const` qualifier prohibits their modification in the function.
 
 - Arguments passed by non-`const` reference can be both read and modified by the function, so they are regarded as both inputs and outputs.

And that's it! DepSpawn will analyze in each `spawn` the function parameter types and the memory positions occupied by the arguments and it will automatically launch for execution the function as soon as its arguments are free of dependencies, enabling to exploit the maximum possible parallelism in your application.

### Examples

In the example below

 - The first two spawns can run in parallel because `s.a` and `s.b` occupy disjoint memory positions.
 - The spawned function `g` waits for the two evaluations of `f` to finish because they both modify portions of `s`, the argument to `g`.
 - The last two spawns can operate in parallel because they operate on different data, but they can only start when `g` finishes because it can modify any component of the struct `s`, and these function calls work in fact on members of `s`.

```cpp 
    #include <iostream>
    #include "depspawn/depspawn.h"

	using namespace depspawn;
	
	struct t_struct {
	  int a, b;
	};
	
	void f(int &i) {
	  i++;
	}
	
	void g(t_struct& s) {  
	  s.a++;
	}
	
	int main()
	{ t_struct s;

	  s.a = 0;
	  s.b = 0;
	  
	  set_threads();
	  
	  spawn(f, s.a); //Spawn 1: sets s.a to 1
	  spawn(f, s.b); //Spawn 2: sets s.b to 1
	  spawn(g, s);   //Spawn 3: sets s.a to 2
	  spawn(f, s.a); //Spawn 4: sets s.a to 3
	  spawn(f, s.b); //Spawn 5: sets s.b to 2
	  
	  wait_for_all();
	
	  bool test_ok = ((s.a == 3) && (s.b == 2));
	  
	  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
	  
	  return !test_ok;
	}
```

Computing a histogram exploiting parallelism is challenging because it implies reductions of array positions obtained through indirections, so the dependencies between the reductions are only known at runtime. In our case, DepSpawn takes care of the problem:

```cpp
	#include <iostream>
	#include <random>
	#include <array>
	#include <algorithm>
	#include "depspawn/depspawn.h"
	
	using namespace depspawn;
	
	static const int N = 100000, RANGE = 100;
	
	std::array<int, N> v;
	std::array<int, RANGE> hist_par, hist_seq;
	
	void incr(int &i) {
	  i++;
	}
	
	int main()
	{
	  // Make some random numbers in the interval 0 -- RANGE -1
	  std::random_device rd;
	  std::uniform_int_distribution<int> dist(0, RANGE - 1);
	  
	  for (int i = 0; i < N; i++) {
	    v[i] = dist(rd);
	  }
	  
	  // Compute the histogram sequentially
	  hist_seq.fill(0);
	  for (int i = 0; i < N; i++)
	    hist_seq[v[i]]++;
	    	
	  // Compute the histogram in parallel
	  hist_par.fill(0);
	  
	  set_threads();
	
	  for (int i = 0; i < N; i++) {
	    spawn(incr, hist_par[v[i]]);
	  }
	  
	  wait_for_all();
		
	  bool test_ok = std::equal(hist_seq.begin(), hist_seq.end(), hist_par.begin());
	  
	  std::cout << "TEST " << (test_ok ? "SUCCESSFUL" : "UNSUCCESSFUL") << std::endl;
	  
	  return !test_ok;
	}
```

### Documentation

The best source for starting with DepSpawn is its [online documentation](http://fraguela.github.io/depspawn/), which includes detailed installation instructions, a step-by-step introduction that covers its semantics and API and several tips and tricks.

A more detailed description of the DepSpawn programming model can be found the publication [A framework for argument-based task synchronization with automatic detection of dependencies](http://www.des.udc.es/~basilio/papers/Gonzalez13-DepSpawn.pdf) ([DOI 10.1016/j.parco.2013.04.012](http://dx.doi.org/10.1016/j.parco.2013.04.012)), although it lacks some of the new synchronization mechanisms added in version 1.0.

### License

The source for DepSpawn is licensed under the [GPLv2 license](http://www.gnu.org/licenses/old-licenses/gpl-2.0.html) with the [runtime exception](http://www.gnu.org/licenses/gcc-exception-faq.html) simply because [that is the license for the Intel® TBB](https://www.threadingbuildingblocks.org/licensing) threading system it relies on. If you change it to rely on any other threading system, feel free to adapt the license accordingly.
