# Makefile for libsmbase

# main target
THISLIBRARY = libsmbase.a
all: ${THISLIBRARY}

# when uncommented, the program emits profiling info
#ccflags = -pg

# pull in basic stuff
include Makefile.base.mk

# delete compiling/editing byproducts
clean:
	rm -f *.o *~

veryclean: clean
	rm -f ${tests-files}
	rm -f *.a

# -------------- main target --------------
# library itself
library-objs = \
  breaker.o crc.o datablok.o exc.o missing.o nonport.o str.o \
  syserr.o voidlist.o warn.o bit2d.o point.o growbuf.o strtokp.o \
  strutil.o strdict.o
${THISLIBRARY}: ${library-objs}
	${makelib} libsmbase.a ${library-objs}
	${ranlib} libsmbase.a

# ---------- module tests ----------------
# test program targets
tests-files = nonport voidlist tobjlist bit2d growbuf
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

check: ${tests-files}
	./nonport
	./voidlist
	./tobjlist
	./bit2d
	./growbuf
	./strdict
	@echo
	@echo "make check: all the tests PASSED"

# end of Makefile
