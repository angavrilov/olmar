# Makefile for parser generator project

# main targets
all: glr lexer1 lexer2

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

lexer1yy.cc: lexer1.lex lexer1.h
	flex lexer1.lex

# first pass lexer
lexer1: lexer1.cc lexer1yy.o trace.o ${SMBASE}/libsmbase.a
	${link} -o lexer1 -DTEST_LEXER1 lexer1.cc lexer1yy.o trace.o ${linkend}

# second pass lexer
lexer2: lexer2.cc lexer1.o lexer1yy.o trace.o
	${link} -o lexer2 -DTEST_LEXER2 lexer2.cc lexer1.o lexer1yy.o trace.o ${linkend}

clean:
	rm -f grammar glr lexer1 lexer1yy.cc *.o *~
