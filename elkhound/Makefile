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
grammar: grammar.h grammar.cc
	${link} -o grammar grammar.cc ${linkend}

