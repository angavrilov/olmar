# Makefile for cdecl example


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
CPPFLAGS := -I$(PARSGEN) -I$(PARSGEN)/cc -I$(AST) -I$(SMBASE)

# C++ compilation flags
CCFLAGS := -g -Wall

# linking flags
LDFLAGS := $(LIBELKHOUND) $(SMBASE)/libsmbase.a


# -------- targets --------
# main target
all: cdecl

# extra object files
extras := \
  $(PARSGEN)/cc/parssppt.o \
  $(PARSGEN)/cc/lexer2.o \
  $(PARSGEN)/cc/lexer1.o \
  $(PARSGEN)/cc/lexer1yy.o \
  $(PARSGEN)/cc/cc_lang.o \
  $(PARSGEN)/asthelp.o \
  $(PARSGEN)/strtable.o

# invoke the parser generator
cdecl.gr.gen.cc cdecl.gr.gen.h: cdecl.gr
	$(ELKHOUND) -v -o cdecl.gr.gen cdecl.gr

# compile a C++ source file
%.o: %.c
	g++ -c $(CCFLAGS) $(CPPFLAGS) $*.cc

# link the driver code with the generated parser and lexer;
# also run it with a test input
cdecl: $(PARSGEN)/glrmain.o cdecl.gr.gen.o
	g++ -g -o $@ $^ $(extras) $(LDFLAGS)
	./cdecl $(PARSGEN)/in/cdecl.in1

clean:
	rm -f cdecl *.bin *.gen.* *.o gdb.log
