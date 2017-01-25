#DepSpawn: Data Dependent Spawn library
#Copyright (C) 2012-2017 Carlos H. Gonzalez, Basilio B. Fraguela. Universidade da Coruna
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
# limitations under the License


# -DDEPSPAWN_FAST_START : Launcher threads executes tasks when too many are enqueued
# -DDEPSPAWN_DUMMY_POOL  : Do not actually use a pool for the internal objects of the DepSpawn runtime
# -DDEPSPAWN_SCALABLE_POOL : false -> use std::malloc/free; true -> use TBB scalable_malloc/scalable_free
# -DDEPSPAWN_ALT        : Use an alternative algorithm for the insertion and release of tasks
# -DDEPSPAWN_PROFILE    : Do profiling


CXX=g++
CXXFLAGS=-O3 -DNDEBUG -std=c++0x

ifndef DEPSPAWN_DIR
  DEPSPAWN_DIR := ..
endif

BLITZ_DIR := $(DEPSPAWN_DIR)/depspawn-blitz-0.10
DEPSPAWN_INCLUDE_DIR := $(DEPSPAWN_DIR)/include
DEPSPAWN_HEADERS := $(DEPSPAWN_INCLUDE_DIR)/depspawn/depspawn.h $(DEPSPAWN_INCLUDE_DIR)/depspawn/workitem.h $(DEPSPAWN_INCLUDE_DIR)/depspawn/workitem_alt.h $(DEPSPAWN_INCLUDE_DIR)/depspawn/DummyLinkedListPool.h $(DEPSPAWN_INCLUDE_DIR)/depspawn/LinkedListPool.h $(DEPSPAWN_INCLUDE_DIR)/depspawn/depspawn_utils.h

NOBLITZTESTBINARIES := super_simple_test mxv_basic test_dep_for_basicchild_stdfunc test_dep_for_basicchild_boostfunc test_by_value test_lambda test_subobject test_wait_children_dep test_out_scope test_donot_wait_for_ancestors test_int test_array test_wait_for test_Observer test_depspawn_sync histogram
BLITZTESTBINARIES   := mxm mxv test_blitz_array testreduction1_fix1

TESTBINARIES := $(NOBLITZTESTBINARIES) $(BLITZTESTBINARIES)
NOTESTBINARIES := testreduction1_orix mxv_basic_orix test_ignore test_array_sizes bench_spawn fib

BINARIES := $(TESTBINARIES) $(NOTESTBINARIES)

# "test_rvalueref" must currently break

all : $(BINARIES)

test : $(TESTBINARIES)
	-@for a in $(TESTBINARIES) ; do            \
	  echo test $$a :;                         \
	  if ! ./$$a;  then                        \
	    echo '*Please report this error*';     \
	    exit;                                  \
	  fi;                                      \
	  echo ------------------------;           \
	done

$(DEPSPAWN_DIR)/src/depspawn.o : $(DEPSPAWN_DIR)/src/depspawn.cpp $(DEPSPAWN_DIR)/src/workitem.cpp $(DEPSPAWN_DIR)/src/workitem_alt.cpp $(DEPSPAWN_HEADERS) Makefile
	$(CXX) -c $(CXXFLAGS) -I$(DEPSPAWN_INCLUDE_DIR) -o $@ $<

$(NOBLITZTESTBINARIES) : % : %.cpp $(DEPSPAWN_DIR)/src/depspawn.o $(DEPSPAWN_HEADERS) Makefile
	$(CXX) $(CXXFLAGS) -I$(DEPSPAWN_INCLUDE_DIR) -o $@ $< $(DEPSPAWN_DIR)/src/depspawn.o -ltbb -ltbbmalloc

$(BLITZTESTBINARIES) $(NOTESTBINARIES) : % : %.cpp $(DEPSPAWN_DIR)/src/depspawn.o $(DEPSPAWN_HEADERS) Makefile
	$(CXX) $(CXXFLAGS) -I$(DEPSPAWN_INCLUDE_DIR) -I$(BLITZ_DIR) -o $@ $< $(DEPSPAWN_DIR)/src/depspawn.o -ltbb -ltbbmalloc

clean:
	-@rm -rf *.o $(DEPSPAWN_DIR)/src/depspawn.o $(BINARIES) *.dSYM

runall:
	$(foreach var,$(BINARIES),echo -e "\033[32m$(var)\033[37m";./$(var);echo;)
	echo -e "\033[m"
