# Makefile for libsmbase

# main target
THISLIBRARY = libsmbase.a
all: gensrc ${THISLIBRARY}

# when uncommented, the program emits profiling info
#ccflags = -pg

# optimizer...
#ccflags = -O2

# pull in basic stuff
include Makefile.base.mk

# delete compiling/editing byproducts
clean:
	rm -f *.o *~

veryclean: clean
	rm -f ${tests-files}
	rm -f *.a


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
malloc.o: malloc.c
	gcc -c -g -O3 -DDEBUG malloc.c

# mysig needs some flags to *not* be set ....
mysig.o: mysig.cc
	gcc -c -g mysig.cc

# library itself
library-objs = \
  breaker.o crc.o datablok.o exc.o missing.o nonport.o str.o \
  syserr.o voidlist.o warn.o bit2d.o point.o growbuf.o strtokp.o \
  strutil.o strdict.o svdict.o strhash.o hashtbl.o malloc.o \
  trdelete.o flatten.o bflatten.o mysig.o trace.o
${THISLIBRARY}: ${library-objs}
	${makelib} libsmbase.a ${library-objs}
	${ranlib} libsmbase.a

# ---------- module tests ----------------
# test program targets
tests-files = nonport voidlist tobjlist bit2d growbuf testmalloc
tests: ${tests-files}

nonport: nonport.cpp nonport.h
	${link} -o nonport -DTEST_NONPORT nonport.cpp ${linkend}

voidlist: voidlist.cc voidlist.h ${THISLIBRARY}
	${link} -o voidlist -DTEST_VOIDLIST voidlist.cc ${THISLIBRARY} ${linkend}

tobjlist: tobjlist.cc objlist.h voidlist.o ${THISLIBRARY}
	${link} -o tobjlist tobjlist.cc voidlist.o ${THISLIBRARY} ${linkend}

bit2d: bit2d.cc bit2d.h ${THISLIBRARY}
	${link} -o bit2d -DTEST_BIT2D bit2d.cc ${THISLIBRARY} ${linkend}

growbuf: growbuf.cc growbuf.h ${THISLIBRARY}
	${link} -o growbuf -DTEST_GROWBUF growbuf.cc ${THISLIBRARY} ${linkend}

strdict: strdict.cc strdict.h ${THISLIBRARY}
	${link} -o strdict -DTEST_STRDICT strdict.cc ${THISLIBRARY} ${linkend}

svdict: svdict.cc svdict.h ${THISLIBRARY}
	${link} -o svdict -DTEST_SVDICT svdict.cc ${THISLIBRARY} ${linkend}

str: str.cpp str.h ${THISLIBRARY}
	${link} -o str -DTEST_STR str.cpp ${THISLIBRARY} ${linkend}

strhash: strhash.cc strhash.h ${THISLIBRARY}
	${link} -o strhash -DTEST_STRHASH strhash.cc ${THISLIBRARY} ${linkend}

trdelete: trdelete.cc trdelete.h ${THISLIBRARY}
	${link} -o trdelete -DTEST_TRDELETE trdelete.cc ${THISLIBRARY} ${linkend}

bflatten: bflatten.cc bflatten.h ${THISLIBRARY}
	${link} -o bflatten -DTEST_BFLATTEN bflatten.cc ${THISLIBRARY} ${linkend}

mysig: mysig.cc mysig.h ${THISLIBRARY}
	gcc -Wall -g -o mysig -DTEST_MYSIG mysig.cc ${THISLIBRARY} ${linkend}

testmalloc: testmalloc.cc ${THISLIBRARY}
	gcc -Wall -g -o testmalloc testmalloc.cc ${THISLIBRARY} ${linkend}

check: ${tests-files}
	./nonport
	./voidlist
	./tobjlist
	./bit2d
	./growbuf
	./strdict
	./svdict
	./str
	./strhash
	./trdelete
	./bflatten
	./mysig
	./testmalloc 2>&1 | tail
	@echo
	@echo "make check: all the tests PASSED"

# end of Makefile
