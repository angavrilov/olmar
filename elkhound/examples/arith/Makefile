# Makefile for arith example


# -------- external tools and libraries --------
# paths to the relevant subsystems
ELKHOUND := ../..
AST      := ../../../ast
SMBASE   := ../../../smbase

# Elkhound parser generator
GRAMANL := $(ELKHOUND)/gramanl

# Elkhound runtime support library
LIBGLR := $(ELKHOUND)/libglr.a


# -------- compiler and linker configuration --------
# preprocessing flags
CPPFLAGS := -I$(ELKHOUND) -I$(AST) -I$(SMBASE)

# C++ compilation flags
CCFLAGS := -g -Wall

# linking flags
LDFLAGS := $(LIBGLR) $(SMBASE)/libsmbase.a


# -------- targets --------
# main target
all: arith

# invoke the parser generator
arith.gr.gen.cc arith.gr.gen.h: arith.gr
	$(GRAMANL) arith

# invoke the lexer generator
arithyy.cc: arith.lex
	flex -o$@ arith.lex

# compile a C++ source file
%.o: %.c
	g++ -c $(CCFLAGS) $(CPPFLAGS) $*.cc

# link the driver code with the generated parser and lexer;
# also run it with a test input
arith: arith.o arithyy.o arith.gr.gen.o
	g++ -o $@ -g $^ $(LDFLAGS)
	echo "3 + 4 * 5" | ./arith 


clean:
	rm -f arith *.bin *.gen.* arithyy.cc *.o
