# Makefile for parser generator project

# main target
all: glr

# directories
SMBASE = /home/scott/wrk/lib/smbase

# dependencies
libraries = ${SMBASE}/libsmbase.a
includes = -I${SMBASE}

# pull in base stuff
include ${SMBASE}/Makefile.base.mk

# ------ targets -------
# SLR(1) parser
grammar: grammar.h grammar.cc gramanl.h gramanl.cc
	${link} -o grammar -DGRAMANL_MAIN grammar.cc gramanl.cc ${linkend}

# GLR parser built on top of SLR(1) parser
glr-dep = grammar.o gramanl.o glr.cc glrtree.o trace.o attr.o cond.o \
          attrexpr.o action.o
glr: ${glr-dep}	glr.h ${SMBASE}/libsmbase.a
	${link} -o glr -DGLR_MAIN ${glr-dep} ${linkend}

clean:
	rm -f grammar glr *.o *~
