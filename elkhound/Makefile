# Makefile for parser generator project

# main target
all: grammar

# directories
SMBASE = /home/scott/wrk/lib/smbase

# dependencies
libraries = ${SMBASE}/libsmbase.a
includes = -I${SMBASE}

# pull in base stuff
include ${SMBASE}/Makefile.base.mk

# ------ targets -------
# my parser!
grammar: grammar.h grammar.cc gramanl.h gramanl.cc
	${link} -o grammar -DGRAMANL_MAIN grammar.cc gramanl.cc ${linkend}

glr: grammar.o gramanl.o glr.h glr.cc
	${link} -o glr -DGLR_MAIN grammar.o gramanl.o glr.cc ${linkend}
