# Makefile for libsmbase

# main target
THISLIBRARY = libsmbase.a
all: gensrc $(THISLIBRARY)

# translation process variables
libraries   := 
includes    := 
ccflags     := -g -Wall -D__LINUX__ -D__UNIX__
no-warnings := -w
makelib     := ar -r
ranlib      := ranlib

# make warnings into errors so I always get a chance to fix them
ccflags += -Werror

# when uncommented, we get profiling info
#ccflags += -pg

# optimizer...
ccflags += -O2 -DNDEBUG

compile := g++ -c $(ccflags) $(includes)
link    := g++ $(ccflags) $(includes)
linkend := $(libraries)

# compile .cc to .o
%.o : %.cc
	$(compile) $< -o $@
	@./depend.sh $(ccflags) $(includes) $< > $*.d

%.o : %.cpp
	$(compile) $< -o $@
	@./depend.sh $(ccflags) $(includes) $< > $*.d

%.o : %.c
	gcc -c $(ccflags) $(includes) $< -o $@

# delete compiling/editing byproducts
clean:
	rm -f *.o *~ *.d

veryclean: clean
	rm -f $(tests-files)
	rm -f *.a

# remove crap that vc makes
vc-clean:
	rm -f *.plg *.[ip]db *.pch

# -------- experimenting with m4 for related files -------
# I don't delete these during make clean because I don't want
# to force people to have m4 installed
gensrc: sobjlist.h objlist.h

sobjlist.h: xobjlist.h
	rm -f sobjlist.h
	m4 -Dm4_output=sobjlist.h --prefix-builtins xobjlist.h > sobjlist.h
	chmod a-w sobjlist.h

objlist.h: xobjlist.h
	rm -f objlist.h
	m4 -Dm4_output=objlist.h --prefix-builtins xobjlist.h > objlist.h
	chmod a-w objlist.h

# -------------- main target --------------
# testing a new malloc
# add the -DDEBUG flag to turn on additional checks
# add the -DTRACE_MALLOC_CALLS flag to print on every alloc/dealloc
# normally -O3 is specified
malloc.o: malloc.c
	gcc -c -g -O3 -DNO_DEBUG -DNO_TRACE_MALLOC_CALLS -DNO_DEBUG_HEAP malloc.c

# mysig needs some flags to *not* be set ....
mysig.o: mysig.cc mysig.h
	gcc -c -g mysig.cc

# library itself
library-objs := \
  breaker.o crc.o datablok.o exc.o missing.o nonport.o str.o \
  syserr.o voidlist.o warn.o bit2d.o point.o growbuf.o strtokp.o \
  strutil.o strdict.o svdict.o strhash.o hashtbl.o malloc.o \
  trdelete.o flatten.o bflatten.o mysig.o trace.o vdtllist.o \
  stringset.o mypopen.o unixutil.o cycles.o
-include $(library-objs:.o=.d)

$(THISLIBRARY): $(library-objs)
	$(makelib) libsmbase.a $(library-objs)
	$(ranlib) libsmbase.a

# ---------- module tests ----------------
# test program targets
tests-files := nonport voidlist tobjlist bit2d growbuf testmalloc mypopen \
               strdict svdict str strutil trdelete bflatten mysig \
               testmalloc mypopen tobjpool strhash cycles
tests: $(tests-files)

nonport: nonport.cpp nonport.h
	$(link) -o nonport -DTEST_NONPORT nonport.cpp $(linkend)

voidlist: voidlist.cc voidlist.h $(THISLIBRARY)
	$(link) -o voidlist -DTEST_VOIDLIST voidlist.cc $(THISLIBRARY) $(linkend)

tobjlist: tobjlist.cc objlist.h voidlist.o $(THISLIBRARY)
	$(link) -o tobjlist tobjlist.cc voidlist.o $(THISLIBRARY) $(linkend)

bit2d: bit2d.cc bit2d.h $(THISLIBRARY)
	$(link) -o bit2d -DTEST_BIT2D bit2d.cc $(THISLIBRARY) $(linkend)

growbuf: growbuf.cc growbuf.h $(THISLIBRARY)
	$(link) -o growbuf -DTEST_GROWBUF growbuf.cc $(THISLIBRARY) $(linkend)

strdict: strdict.cc strdict.h $(THISLIBRARY)
	$(link) -o strdict -DTEST_STRDICT strdict.cc $(THISLIBRARY) $(linkend)

svdict: svdict.cc svdict.h $(THISLIBRARY)
	$(link) -o svdict -DTEST_SVDICT svdict.cc $(THISLIBRARY) $(linkend)

str: str.cpp str.h $(THISLIBRARY)
	$(link) -o str -DTEST_STR str.cpp $(THISLIBRARY) $(linkend)

strutil: strutil.cc strutil.h $(THISLIBRARY)
	$(link) -o strutil -DTEST_STRUTIL strutil.cc $(THISLIBRARY) $(linkend)

strhash: strhash.cc strhash.h $(THISLIBRARY)
	$(link) -o strhash -DTEST_STRHASH strhash.cc $(THISLIBRARY) $(linkend)

trdelete: trdelete.cc trdelete.h $(THISLIBRARY)
	$(link) -o trdelete -DTEST_TRDELETE trdelete.cc $(THISLIBRARY) $(linkend)

bflatten: bflatten.cc bflatten.h $(THISLIBRARY)
	$(link) -o bflatten -DTEST_BFLATTEN bflatten.cc $(THISLIBRARY) $(linkend)

mysig: mysig.cc mysig.h $(THISLIBRARY)
	gcc -Wall -g -o mysig -DTEST_MYSIG mysig.cc $(THISLIBRARY) $(linkend)

testmalloc: testmalloc.cc $(THISLIBRARY)
	gcc -Wall -g -o testmalloc testmalloc.cc $(THISLIBRARY) $(linkend)

mypopen: mypopen.c mypopen.h
	g++ -Wall -g -o mypopen -DTEST_MYPOPEN mypopen.c

# this test is only useful when malloc is compiled with DEBUG_HEAP
tmalloc: tmalloc.c
	gcc -Wall -g -o tmalloc tmalloc.c $(THISLIBRARY)

tobjpool: tobjpool.cc objpool.h
	g++ -Wall -g -o tobjpool tobjpool.cc $(THISLIBRARY)

cycles: cycles.h cycles.c
	gcc -Wall -g -o cycles -DTEST_CYCLES cycles.c

crc: crc.cpp
	gcc -Wall -g -o crc -DTEST_CRC crc.cpp

check: $(tests-files)
	./nonport
	./voidlist
	./tobjlist
	./bit2d
	./growbuf
	./strdict
	./svdict
	./str
	./strutil
	./strhash
	./trdelete
	./bflatten
	./mysig
	./testmalloc 2>&1 | tail
	./mypopen
	./tobjpool
	./cycles
	@echo
	@echo "make check: all the tests PASSED"

# end of Makefile
