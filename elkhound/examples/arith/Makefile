# Makefile for arith example


# -------- external tools and libraries --------
# paths to the relevant subsystems
ELKHOUND_DIST := ../../..
SMBASE        := $(ELKHOUND_DIST)/smbase
AST           := $(ELKHOUND_DIST)/ast
PARSGEN       := $(ELKHOUND_DIST)/parsgen

# Elkhound parser generator
ELKHOUND := $(PARSGEN)/elkhound

# Elkhound runtime support library
LIBELKHOUND := $(PARSGEN)/libelkhound.a


# -------- compiler and linker configuration --------
# preprocessing flags
CPPFLAGS := -I$(PARSGEN) -I$(AST) -I$(SMBASE)

# C++ compilation flags
CCFLAGS := -g -Wall

# linking flags
LDFLAGS := $(LIBELKHOUND) $(SMBASE)/libsmbase.a


# -------- targets --------
# main target
all: arith

# invoke the parser generator
arith.gr.gen.cc arith.gr.gen.h: arith.gr
	$(ELKHOUND) arith

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
