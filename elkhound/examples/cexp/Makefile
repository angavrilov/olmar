# Makefile for cexp examples


# -------- external tools and libraries --------
# paths to the relevant subsystems
ELKHOUND := ../..
AST      := ../../../ast
SMBASE   := ../../../smbase

# Elkhound parser generator
GRAMANL := $(ELKHOUND)/gramanl


# -------- compiler and linker configuration --------
# preprocessing flags
CPPFLAGS := -I$(ELKHOUND) -I$(ELKHOUND)/cc -I$(AST) -I$(SMBASE)

# C++ compilation flags
CCFLAGS := -g -Wall

# linking flags; I'm cheating a bit here by pulling in all of
# libccgr.a (thereby borrowing much of the C/C++ infrastructure)
# instead of just libglr.a
LDFLAGS := $(ELKHOUND)/cc/libccgr.a $(SMBASE)/libsmbase.a


# -------- generic rules --------
# invoke the parser generator
%.gr.gen.cc %.gr.gen.h: %.gr
	$(GRAMANL) $*

# invoke the AST generator
%.ast.gen.cc %.ast.gen.h: %.ast
	$(AST)/astgen -b$*.ast.gen $*.ast

# compile a C++ source file
%.o: %.c
	g++ -c $(CCFLAGS) $(CPPFLAGS) $*.cc


# -------- targets --------
# main target
all: cexp3ast.ast.gen.h cexp3 cexp3b

# cexp3b: disambiguation using 'merge'
cexp3b.gr: cexp3.gr
	rm -f $@
	grep -v PREC cexp3.gr >$@
	chmod a-w $@

cexp3: $(ELKHOUND)/glrmain.o cexp3.gr.gen.o cexp3mrg.o cexp3ast.ast.gen.o 
	g++ -g -o $@ $^ $(LDFLAGS)
	./$@ cexp3.bin $(ELKHOUND)/in/cexp3.in1 | tee tmp.out
	grep 'result: 7' tmp.out
	rm tmp.out

cexp3b: $(ELKHOUND)/glrmain.o cexp3b.gr.gen.o cexp3mrg.o cexp3ast.ast.gen.o
	g++ -g -o $@ $^ $(LDFLAGS)
	./$@ cexp3b.bin $(ELKHOUND)/in/cexp3.in1 | tee tmp.out
	grep 'result: 7' tmp.out
	rm tmp.out


clean:
	rm -f cexp3 cexp3b cexp3b.gr *.bin *.gen.* *.o
