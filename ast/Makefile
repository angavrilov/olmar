# Makefile for ast project
# see license.txt for copyright and terms of use

SMBASE  := ../smbase

CXX      := g++
CXXFLAGS := -Wall -g

# for gcc-2
#CXXFLAGS += -I.

CXXFLAGS += -I$(SMBASE) $(EXTRAARGS)

CXXFLAGS += -Werror


# to enable profiling
#CXXFLAGS += -pg

# for gcc-3
CXXFLAGS += -Wno-deprecated

# to turn on optimizer
#CXXFLAGS += -O2 -DNDEBUG

COMPILE  := $(CXX) -c $(CXXFLAGS)


LDFLAGS   := -Wall -g
LINK      := $(CXX) $(LDFLAGS)
LIBSMBASE := $(SMBASE)/libsmbase.a
LINKEND   := $(LIBSMBASE)

# default target
all: agrampar astgen example.o ext1.o

# unfortunately, the way bison.simple declares YYSTYPE precludes a proper
# declaration of yylex(), unless I modify it as below; the 'grep' is so
# I have a .h file without the union, since the union refers to types
# whose declarations I'd like to avoid spreading everywhere
agrampar.tab.cc agrampar.tab.h agrampar.codes.h: agrampar.y ast.hand.h ast.ast.h agrampar.h
	bison -d -v agrampar.y
	mv -f agrampar.tab.c tmp
	sed 's/typedef union {/typedef union YYSTYPE {/' < tmp > agrampar.tab.cc
	mv -f agrampar.tab.h tmp
	sed 's/typedef union {/typedef union YYSTYPE {/' < tmp > agrampar.tab.h
	rm tmp
	grep '# *define' agrampar.tab.h > agrampar.codes.h

# have to use sed to change the forward declaration of 'istream'
# into an #include, so the std:: namespace doesn't cause a problem
agramlex.yy.cc: gramlex.h agramlex.lex agrampar.codes.h
	flex -o$@ agramlex.lex
	mv $@ agramlex.tmp
	sed 's/class istream;/#include <iostream.h>/' <agramlex.tmp >$@
	rm agramlex.tmp

#$(PARSGEN)/%.o: $(PARSGEN)/%.cc
#	make -C $(PARSGEN) check-makefiles $*.o

%.o: %.cc
	$(COMPILE) $*.cc
	@$(SMBASE)/depend.sh $(CXXFLAGS) $< > $*.d

# removed fileloc.o
agrampar-dep = gramlex.o agrampar.tab.o ccsstr.o \
               agramlex.yy.o embedded.o ast.hand.o \
               asthelp.o strtable.o
-include $(agrampar-dep:.o=.d)

agrampar: $(agrampar-dep) agrampar.cc ast.ast.cc $(LIBSMBASE) 
	$(LINK) -o agrampar $(CXXFLAGS) $(agrampar-dep) \
                -DTEST_AGRAMPAR agrampar.cc $(LINKEND)

# ast.ast.cc is a dependency here but not explicitly in the command
# line because ast.hand.cc #includes it
astgen: $(agrampar-dep) agrampar.o astgen.o ast.ast.cc $(LIBSMBASE) 
	$(LINK) -o astgen $(agrampar-dep) agrampar.o astgen.o $(LINKEND)

# test code for ccsstr
ccsstr: ccsstr.cc ccsstr.h reporterr.o embedded.o
	$(LINK) -o ccsstr -DTEST_CCSSTR ccsstr.cc \
                reporterr.o embedded.o $(LINKEND)

# simple ast spec file
example.cc: astgen example.ast
	./astgen example.ast

exampletest: exampletest.o example.o asthelp.o locstr.o strtable.o $(LIBSMBASE)
	$(LINK) -o $@ $^

# simple extension
ext1.cc: astgen example.ast ext1.ast
	./astgen -oext1 example.ast ext1.ast

# If you want to regenerate astgen's own ast file, you do
#   ./astgen -oast.ast ast.ast
#
# This rule is *not* in the Makefile because if you do it
# without thinking you can break the self-bootstrapping
# (and be forced to restore your ast.ast.{cc,h} from the
# distribution tarball or from CVS).

clean:
	rm -f *.o agrampar.tab.* tmp *.d
	rm -f agrampar astgen ccsstr towner exampletest
	rm -f agrampar.{codes.h,output} agramlex.yy.cc

# test for owner
towner: owner.h towner.cc
	$(COMPILE) towner.cc
	$(LINK) -o towner towner.o


