# Makefile for cdecl example


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


# -------- targets --------
# main target
all: cdecl

# invoke the parser generator
cdecl.gr.gen.cc cdecl.gr.gen.h: cdecl.gr
	$(GRAMANL) cdecl

# compile a C++ source file
%.o: %.c
	g++ -c $(CCFLAGS) $(CPPFLAGS) $*.cc

# link the driver code with the generated parser and lexer;
# also run it with a test input
cdecl: $(ELKHOUND)/glrmain.o cdecl.gr.gen.o
	g++ -g -o $@ $^ $(LDFLAGS)
	./cdecl cdecl.bin $(ELKHOUND)/in/cdecl.in1

clean:
	rm -f cdecl *.bin *.gen.* *.o gdb.log
