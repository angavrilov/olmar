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
glr: grammar.o gramanl.o glr.h glr.cc glrtree.o
	${link} -o glr -DGLR_MAIN grammar.o gramanl.o glrtree.o glr.cc ${linkend}

clean:
	rm -f grammar glr *.o *~
