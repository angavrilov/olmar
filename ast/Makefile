# Makefile for ast project

SMBASE  := ../smbase

CXX      := g++
CXXFLAGS := -Wall -g -I. -I$(SMBASE) $(EXTRAARGS)
CXXFLAGS += -Werror
COMPILE  := $(CXX) -c $(CXXFLAGS)

LDFLAGS   := -Wall -g
LINK      := $(CXX) $(LDFLAGS)
LIBSMBASE := $(SMBASE)/libsmbase.a
LINKEND   := $(LIBSMBASE)

# default target
all: agrampar astgen

# unfortunately, the way bison.simple declares YYSTYPE precludes a proper
# declaration of yylex(), unless I modify it as below; the 'grep' is so
# I have a .h file without the union, since the union refers to types
# whose declarations I'd like to avoid spreading everywhere
agrampar.tab.cc agrampar.tab.h agrampar.codes.h: agrampar.y ast.hand.h agrampar.h
	bison -d -v agrampar.y
	mv -f agrampar.tab.c tmp
	sed 's/typedef union {/typedef union YYSTYPE {/' < tmp > agrampar.tab.cc
	mv -f agrampar.tab.h tmp
	sed 's/typedef union {/typedef union YYSTYPE {/' < tmp > agrampar.tab.h
	rm tmp
	grep '#define' agrampar.tab.h > agrampar.codes.h

agramlex.yy.cc: gramlex.h agramlex.lex agrampar.codes.h
	flex -oagramlex.yy.cc agramlex.lex

#$(PARSGEN)/%.o: $(PARSGEN)/%.cc
#	make -C $(PARSGEN) check-makefiles $*.o

%.o: %.cc
	$(COMPILE) $*.cc
	@$(SMBASE)/depend.sh $(CXXFLAGS) $< > $*.d

agrampar-dep = gramlex.o agrampar.tab.o ccsstr.o \
               fileloc.o agramlex.yy.o embedded.o ast.hand.o \
               asthelp.o strtable.o
-include $(agrampar-dep:.o=.d)

agrampar: $(agrampar-dep) agrampar.cc $(LIBSMBASE) 
	$(LINK) -o agrampar $(CXXFLAGS) $(agrampar-dep) \
                -DTEST_AGRAMPAR agrampar.cc $(LINKEND)

astgen: $(agrampar-dep) agrampar.o astgen.o $(LIBSMBASE) 
	$(LINK) -o astgen $(agrampar-dep) agrampar.o astgen.o $(LINKEND)

# test code for ccsstr
ccsstr: ccsstr.cc ccsstr.h reporterr.o embedded.o
	$(link) -o ccsstr -DTEST_CCSSTR ccsstr.cc \
                reporterr.o embedded.o $(linkend)

clean:
	rm -f *.o agrampar astgen agrampar.tab.* tmp *.d ccsstr towner

# test for owner
towner: owner.h towner.cc
	$(COMPILE) towner.cc
	$(LINK) -o towner towner.o


