# Makefile for parser generator project

# main targets
all: grampar

everything: grammar glr lexer1 lexer2 grampar

# directories
SMBASE = /home/scott/wrk/lib/smbase

# dependencies
libraries = ${SMBASE}/libsmbase.a
includes = -I${SMBASE}
#ccflags = -pg

# pull in base stuff
include ${SMBASE}/Makefile.base.mk

# sets of related object files
grammar-set = grammar.o action.o cond.o attr.o attrexpr.o
common = trace.o

# ------ targets -------
# SLR(1) parser
grammar-dep = gramanl.cc ${grammar-set} ${common}
grammar: ${grammar-dep} grammar.h gramanl.h 
	${link} -o grammar -DGRAMANL_MAIN ${grammar-dep} ${linkend}

# GLR parser built on top of SLR(1) parser
glr-dep = glr.cc ${grammar-set} ${common} gramanl.o glrtree.o \
          lexer1.o lexer1yy.o lexer2.o grampar.o gramlex.yy.o \
          grampar.tab.o gramlex.o ast.o fileloc.o gramast.o
glr: ${glr-dep} glr.h ${SMBASE}/libsmbase.a
	${link} -o glr -DGLR_MAIN ${glr-dep} ${linkend}

lexer1yy.cc: lexer1.lex lexer1.h
	flex lexer1.lex

# first pass lexer
lexer1-dep = lexer1.cc ${common} lexer1yy.o fileloc.o
lexer1: ${lexer1-dep} ${SMBASE}/libsmbase.a
	${link} -o lexer1 -DTEST_LEXER1 ${lexer1-dep} ${linkend}

# second pass lexer
lexer2-dep = lexer2.cc ${common} lexer1.o lexer1yy.o fileloc.o
lexer2: ${lexer2-dep} lexer2.h
	${link} -o lexer2 -DTEST_LEXER2 ${lexer2-dep} ${linkend}

# grammar lexer
gramlex.yy.cc: gramlex.h gramlex.cc gramlex.lex grampar.tab.h
	flex -ogramlex.yy.cc gramlex.lex                     

# grammar lexer test program
gramlex: gramlex.yy.cc gramlex.cc gramlex.h ${common}
	${link} -o gramlex -DTEST_GRAMLEX gramlex.yy.cc gramlex.cc trace.o ${linkend}

# grammar parser
grampar.tab.cc grampar.tab.h: grampar.y
	bison -d -v $^
	mv -f grampar.tab.c grampar.tab.cc

# grammar parser test program
grampar: grampar.cc ${grammar-set} ${common} gramlex.yy.o grampar.tab.o \
         gramlex.o ast.o fileloc.o gramast.o
	${link} -o grampar -DTEST_GRAMPAR $^ ${linkend}

# token list for my grammar
cc.tok: lexer2
	./lexer2 myparser > cc.tok

clean:
	rm -f grammar glr lexer1 lexer1yy.cc gramlex.yy.cc gramlex *.o *~
	rm -f lexer2 grampar.tab.* grampar a.out

# --------------- random other stuff --------------------
# printable version of the C++ grammar; removes the '// C++' tags
# because they just wrap onto next line and mess things up
cc.gr.ps:
	sed 's#[ 	]*// C[+][+].*##' < cc.gr \
	  | enscript -2r --output=cc.gr.ps ; true

# test for owner
towner: owner.h towner.cc
	${link} -o towner towner.cc

