# Makefile for parser generator project

# main targets
all: ccgr

everything: gramanl glr lexer1 lexer2 grampar cexp2 ccgr

# directories
SMBASE = /home/scott/wrk/lib/smbase

# dependencies
libraries = ${SMBASE}/libsmbase.a
includes = -I${SMBASE}
#ccflags = -pg

# pull in base stuff
include ${SMBASE}/Makefile.base.mk

# sets of related object files
grammar-set = grammar.o action.o cond.o attr.o attrexpr.o litcode.o
grampar-set = grampar.tab.o gramlex.yy.o grampar.o gramlex.o \
              ast.o fileloc.o gramast.o ccsstr.o
glr-set = gramanl.o glrtree.o glr.o lexer1.o lexer1yy.o \
          lexer2.o parssppt.o
cc-set = cc_tree.o cc_type.o cc_env.o cc_err.o
common = trace.o

# ------ targets -------
# reads the grammar and emits C++ code for semantic functions
gramanl-dep = gramanl.cc ${grammar-set} ${common} ${grampar-set} \
              ccwrite.o emitcode.o glrtree.o lexer2stub.o
gramanl: ${gramanl-dep} grammar.h gramanl.h 
	${link} -o gramanl -DGRAMANL_MAIN ${gramanl-dep} ${linkend}

# GLR parser built on top of SLR(1) parser
glr-dep = glr.cc ${grammar-set} ${common} ${grampar-set} gramanl.o \
          glrtree.o lexer1.o lexer1yy.o lexer2.o parssppt.o \
          parsstub.o
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
gramlex-dep = gramlex.yy.cc gramlex.cc fileloc.o ccsstr.o ${common}
gramlex: gramlex.h ${gramlex-dep}
	${link} -o gramlex -DTEST_GRAMLEX ${gramlex-dep} ${linkend}

# grammar parser
grampar.tab.cc grampar.tab.h: grampar.y
	bison -d -v $^
	mv -f grampar.tab.c grampar.tab.cc

# grammar parser test program
grampar: grampar.cc ${grammar-set} ${common} gramlex.yy.o grampar.tab.o \
         gramlex.o ast.o fileloc.o gramast.o ccsstr.o glrtree.o \
         lexer2stub.o
	${link} -o grampar -DTEST_GRAMPAR $^ ${linkend}

# token list for my grammar
cc.tok: lexer2
	./lexer2 myparser > cc.tok

# C expression (sort of) evaluator
cexp2-sem-dep = cexp2.sem.cc ${grammar-set} ${common} ${glr-set} \
		grampar.tab.o gramlex.yy.o \
		grampar.o gramlex.o ast.o fileloc.o gramast.o
cexp2.sem: ${cexp2-sem-dep}
	${link} -o cexp2.sem -DTEST_CEXP2_SEM ${cexp2-sem-dep} ${linkend}

# test code for ccsstr
ccsstr: ccsstr.cc ccsstr.h
	${link} -o ccsstr -DTEST_CCSSTR ccsstr.cc ${linkend}

# intermediate files for cexp2
# ('chmod a-w' is so I don't accidentally edit them)
cexp2.cc cexp2.h: cexp2.gr gramanl
	rm -f cexp2.{h,cc}
	./gramanl cexp2
	chmod a-w cexp2.cc cexp2.h

# parser and semantic analyzer for cexp2.gr
cexp2-dep = cexp2.o ${grammar-set} ${common} ${grampar-set} \
            ${glr-set}
cexp2: ${cexp2-dep} cexp2.h
	${link} -o cexp2 ${cexp2-dep} ${linkend}

# intermediate files for ccgr
cc.cc cc.h: cc.gr cc.tok gramanl
	rm -f cc.{h,cc}
	./gramanl cc
	chmod a-w cc.cc cc.h

# parser and semantic analyzer for cc.gr
# (called the target 'ccgr' instead of 'cc' to avoid name collision)
ccgr-dep = cc.o ${grammar-set} ${common} ${grampar-set} \
           ${glr-set} ${cc-set}
ccgr: ${ccgr-dep} cc.h
	${link} -o ccgr ${ccgr-dep} ${linkend}

clean:
	rm -f gramanl glr lexer1 lexer1yy.cc gramlex.yy.cc gramlex *.o *~
	rm -f lexer2 grampar.tab.* grampar a.out
	rm -f cexp2.sem ccsstr cexp2.{h,cc} cexp2 cc.{h,cc} ccgr
	rm -f towner cc.gr.ps

# regression tests
check: cexp2 ccgr
	./cexp2 cexp2.gr cexp2.in2
	./ccgr cc.gr c.in1
	./ccgr cc.gr c.in4
	@echo ""
	@echo "Regression tests passed."

# --------------- random other stuff --------------------
# printable version of the C++ grammar; removes the '// C++' tags
# because they just wrap onto next line and mess things up
cc.gr.ps:
	sed 's#[ 	]*// C[+][+].*##' < cc.gr \
	  | enscript -2r --output=cc.gr.ps ; true

# test for owner
towner: owner.h towner.cc
	${link} -o towner towner.cc

# test for rcptr
trcptr: rcptr.h trcptr.cc
	${link} -o trcptr trcptr.cc
