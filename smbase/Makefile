# Makefile for libsmbase

# main target
THISLIBRARY = libsmbase.a
all: ${THISLIBRARY}

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
  syserr.o voidlist.o warn.o bit2d.o point.o growbuf.o
${THISLIBRARY}: ${library-objs}
	${makelib} libsmbase.a ${library-objs}

# ---------- module tests ----------------
# test program targets
tests-files = nonport voidlist tobjlist bit2d growbuf
tests: ${tests-files}

# test the nonportable routines
nonport: nonport.cpp
	${link} -o nonport -DTEST_NONPORT nonport.cpp ${linkend}

voidlist: voidlist.cc ${THISLIBRARY}
	${link} -o voidlist -DTEST_VOIDLIST voidlist.cc ${THISLIBRARY} ${linkend}

tobjlist: tobjlist.cc voidlist.o ${THISLIBRARY}
	${link} -o tobjlist tobjlist.cc voidlist.o ${THISLIBRARY} ${linkend}

bit2d: bit2d.cc ${THISLIBRARY}
	${link} -o bit2d -DTEST_BIT2D bit2d.cc ${THISLIBRARY} ${linkend}

growbuf: growbuf.cc ${THISLIBRARY}
	${link} -o growbuf -DTEST_GROWBUF growbuf.cc ${THISLIBRARY} ${linkend}

check: ${tests-files}
	nonport
	voidlist
	tobjlist
	bit2d
	growbuf
	@echo
	@echo "make check: all the tests PASSED"

# end of Makefile
