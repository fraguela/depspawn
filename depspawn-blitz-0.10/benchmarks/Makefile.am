#
# Written by Patrick Guio <patrick.guio@fys.uio.no>
#

EXTRA_DIST = arrexpr1.cpp arrexpr1.m cfd.cpp cfdf.f cfortran.h	     \
chunky.cpp compiletime.cpp ctime-results ctime1.cpp ctime1v.cpp	     \
ctime2.cpp ctime2v.cpp ctime3.cpp ctime3v.cpp ctime4.cpp ctime4v.cpp \
ctime5.cpp ctime5c.cpp ctime5v.cpp daxpy2.cpp daxpyf90-2.f90 dot.cpp \
dot2.cpp echof2-back.f echotune.cpp echotune.m echotunef.f frek.m    \
hao-he-mark.cpp kepler.cpp loop1-bug.cpp loop4.cpp loop4f.f	     \
loop4f90.f90 loopstruct.cpp looptest.cpp makelogo.cpp makeloops.cpp  \
qcd.txt quinlan.cpp stenciln.cpp tiny3.cpp iter.cpp


AM_CPPFLAGS = -I$(srcdir) -I$(top_srcdir) -I$(top_builddir) $(BOOST_CPPFLAGS)
AM_CXXFLAGS = @CXX_OPTIMIZE_FLAGS@ @CXXFFLAGS@ @CXXFCFLAG@
AM_FFLAGS = @F77_OPTIMIZE_FLAGS@
AM_FCFLAGS = $(FCFLAGS_f90) @FC_OPTIMIZE_FLAGS@
LDADD = -L$(top_builddir)/lib -lblitz @BLASLIB@ @FCLIBS@ @FLIBS@


############################################################################

LOOP_KERNELS = loop1 loop2 loop3 loop5 loop6 loop8 loop9 loop10 loop11 \
loop12 loop13 loop14 loop15 loop16 loop17 loop18 loop19 loop21 loop22 \
loop23 loop24 loop25 loop36 loop100 \
floop1 floop2 floop3 floop5 floop6 floop8 floop9 floop10 floop11 \
floop12 floop13 floop14 floop15 floop16 floop17 floop18 floop19 floop21 floop22 \
floop23 floop24 floop25 floop36

OTHER_BENCH = arrdaxpy daxpy haney qcd stencil tinydaxpy acoustic acou3d hao-he iter cfd

BENCHMARKS = $(LOOP_KERNELS) $(OTHER_BENCH)

COMPILE_TIME_BENCHMARKS = ctime1 ctime2 ctime3 ctime4 ctime5 ctime1v \
ctime2v ctime3v ctime4v ctime5v

EXTRA_PROGRAMS = $(BENCHMARKS)

#compile: $(EXTRA_PROGRAMS) 

run:	$(OTHER_BENCH)
	@for benchmark in $(OTHER_BENCH) ; do \
	  echo $$benchmark; \
	  ./$$benchmark; \
	done

run-loops:	$(LOOP_KERNELS)
	@for benchmark in $(LOOP_KERNELS) ; do \
	  echo $$benchmark; \
	  ./$$benchmark; \
	done

ctime:
	rm -f $(COMPILE_TIME_BENCHMARKS)
	@echo Flags: $(CXXFLAGS)
	@for benchmark in $(COMPILE_TIME_BENCHMARKS) ; do \
	  echo Compile time for $$benchmark.cpp: ; \
	  time $(CXX) $(AM_CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) -c $(srcdir)/$$benchmark.cpp; \
	done

check-benchmarks: run run-loops ctime

############################################################################

haney_SOURCES = haney.cpp haneyf.f
qcd_SOURCES = qcd.cpp qcdf.f
arrdaxpy_SOURCES = arrdaxpy.cpp arrdaxpyf.f
tinydaxpy_SOURCES = tinydaxpy.cpp
hao_he_SOURCES = hao-he.cpp
iter_SOURCES= iter.cpp
cfd_SOURCES= cfd.cpp

if F90_COMPILER

daxpy_SOURCES = daxpy.cpp fdaxpy.f fidaxpy.f daxpyf90.f90
stencil_SOURCES = stencil.cpp stencilf.f stencilf2.f stencilf90.f90
acoustic_SOURCES = acoustic.cpp acousticf.f acousticf2.f acousticf90.f90 \
acousticf902.f90
acou3d_SOURCES = acou3db1.cpp acou3db2.cpp acou3db3.cpp acou3db4.cpp \
acou3d.cpp acou3df90.f90 acou3df.f acou3df2.f acou3df902.f90

else

daxpy_SOURCES = daxpy.cpp fdaxpy.f fidaxpy.f
stencil_SOURCES = stencil.cpp stencilf.f stencilf2.f
acoustic_SOURCES = acoustic.cpp acousticf.f acousticf2.f
acou3d_SOURCES = acou3db1.cpp acou3db2.cpp acou3db3.cpp acou3db4.cpp \
acou3d.cpp acou3df.f acou3df2.f 

endif

# Loop kernel tests
#
# Note: The loop numbers skip around because they represent
# selected loops from a suite used by IBM researchers to test
# the RS/6000.  Missing loops are ones which were too complicated
# to generate automatically.
#
# This section generated by makeloops.cpp. No, not any longer.

if F90_COMPILER
loop1_SOURCES = loop1.cpp loop1f.f loop1f90.f90
loop2_SOURCES = loop2.cpp loop2f.f loop2f90.f90
loop3_SOURCES = loop3.cpp loop3f.f loop3f90.f90
loop5_SOURCES = loop5.cpp loop5f.f loop5f90.f90
loop6_SOURCES = loop6.cpp loop6f.f loop6f90.f90
loop8_SOURCES = loop8.cpp loop8f.f loop8f90.f90
loop9_SOURCES = loop9.cpp loop9f.f loop9f90.f90
loop10_SOURCES = loop10.cpp loop10f.f loop10f90.f90
loop11_SOURCES = loop11.cpp loop11f.f loop11f90.f90
loop12_SOURCES = loop12.cpp loop12f.f loop12f90.f90
loop13_SOURCES = loop13.cpp loop13f.f loop13f90.f90
loop14_SOURCES = loop14.cpp loop14f.f loop14f90.f90
loop15_SOURCES = loop15.cpp loop15f.f loop15f90.f90
loop16_SOURCES = loop16.cpp loop16f.f loop16f90.f90
loop17_SOURCES = loop17.cpp loop17f.f loop17f90.f90
loop18_SOURCES = loop18.cpp loop18f.f loop18f90.f90
loop19_SOURCES = loop19.cpp loop19f.f loop19f90.f90
loop21_SOURCES = loop21.cpp loop21f.f loop21f90.f90
loop22_SOURCES = loop22.cpp loop22f.f loop22f90.f90
loop23_SOURCES = loop23.cpp loop23f.f loop23f90.f90
loop24_SOURCES = loop24.cpp loop24f.f loop24f90.f90
loop25_SOURCES = loop25.cpp loop25f.f loop25f90.f90
loop36_SOURCES = loop36.cpp loop36f.f loop36f90.f90
loop100_SOURCES = loop100.cpp loop100f.f loop100f90.f90
else
loop1_SOURCES = loop1.cpp loop1f.f
loop2_SOURCES = loop2.cpp loop2f.f
loop3_SOURCES = loop3.cpp loop3f.f
loop5_SOURCES = loop5.cpp loop5f.f
loop6_SOURCES = loop6.cpp loop6f.f
loop8_SOURCES = loop8.cpp loop8f.f
loop9_SOURCES = loop9.cpp loop9f.f
loop10_SOURCES = loop10.cpp loop10f.f
loop11_SOURCES = loop11.cpp loop11f.f
loop12_SOURCES = loop12.cpp loop12f.f
loop13_SOURCES = loop13.cpp loop13f.f
loop14_SOURCES = loop14.cpp loop14f.f
loop15_SOURCES = loop15.cpp loop15f.f
loop16_SOURCES = loop16.cpp loop16f.f
loop17_SOURCES = loop17.cpp loop17f.f
loop18_SOURCES = loop18.cpp loop18f.f
loop19_SOURCES = loop19.cpp loop19f.f
loop21_SOURCES = loop21.cpp loop21f.f
loop22_SOURCES = loop22.cpp loop22f.f
loop23_SOURCES = loop23.cpp loop23f.f
loop24_SOURCES = loop24.cpp loop24f.f
loop25_SOURCES = loop25.cpp loop25f.f
loop36_SOURCES = loop36.cpp loop36f.f
loop100_SOURCES = loop100.cpp loop100f.f
endif

if F90_COMPILER
floop1_SOURCES = floop1.cpp floop1f.f floop1f90.f90
floop2_SOURCES = floop2.cpp floop2f.f floop2f90.f90
floop3_SOURCES = floop3.cpp floop3f.f floop3f90.f90
floop5_SOURCES = floop5.cpp floop5f.f floop5f90.f90
floop6_SOURCES = floop6.cpp floop6f.f floop6f90.f90
floop8_SOURCES = floop8.cpp floop8f.f floop8f90.f90
floop9_SOURCES = floop9.cpp floop9f.f floop9f90.f90
floop10_SOURCES = floop10.cpp floop10f.f floop10f90.f90
floop11_SOURCES = floop11.cpp floop11f.f floop11f90.f90
floop12_SOURCES = floop12.cpp floop12f.f floop12f90.f90
floop13_SOURCES = floop13.cpp floop13f.f floop13f90.f90
floop14_SOURCES = floop14.cpp floop14f.f floop14f90.f90
floop15_SOURCES = floop15.cpp floop15f.f floop15f90.f90
floop16_SOURCES = floop16.cpp floop16f.f floop16f90.f90
floop17_SOURCES = floop17.cpp floop17f.f floop17f90.f90
floop18_SOURCES = floop18.cpp floop18f.f floop18f90.f90
floop19_SOURCES = floop19.cpp floop19f.f floop19f90.f90
floop21_SOURCES = floop21.cpp floop21f.f floop21f90.f90
floop22_SOURCES = floop22.cpp floop22f.f floop22f90.f90
floop23_SOURCES = floop23.cpp floop23f.f floop23f90.f90
floop24_SOURCES = floop24.cpp floop24f.f floop24f90.f90
floop25_SOURCES = floop25.cpp floop25f.f floop25f90.f90
floop36_SOURCES = floop36.cpp floop36f.f floop36f90.f90
else
floop1_SOURCES = floop1.cpp floop1f.f
floop2_SOURCES = floop2.cpp floop2f.f
floop3_SOURCES = floop3.cpp floop3f.f
floop5_SOURCES = floop5.cpp floop5f.f
floop6_SOURCES = floop6.cpp floop6f.f
floop8_SOURCES = floop8.cpp floop8f.f
floop9_SOURCES = floop9.cpp floop9f.f
floop10_SOURCES = floop10.cpp floop10f.f
floop11_SOURCES = floop11.cpp floop11f.f
floop12_SOURCES = floop12.cpp floop12f.f
floop13_SOURCES = floop13.cpp floop13f.f
floop14_SOURCES = floop14.cpp floop14f.f
floop15_SOURCES = floop15.cpp floop15f.f
floop16_SOURCES = floop16.cpp floop16f.f
floop17_SOURCES = floop17.cpp floop17f.f
floop18_SOURCES = floop18.cpp floop18f.f
floop19_SOURCES = floop19.cpp floop19f.f
floop21_SOURCES = floop21.cpp floop21f.f
floop22_SOURCES = floop22.cpp floop22f.f
floop23_SOURCES = floop23.cpp floop23f.f
floop24_SOURCES = floop24.cpp floop24f.f
floop25_SOURCES = floop25.cpp floop25f.f
floop36_SOURCES = floop36.cpp floop36f.f
endif

###########################################################################

clean-local:
	-rm -rf *.ii *.ti *.int.c *.s work.pc* cxx_repository Template.dir ii_files ti_files core.[0-9]* \
		$(BENCHMARKS) $(COMPILE_TIME_BENCHMARKS) 

