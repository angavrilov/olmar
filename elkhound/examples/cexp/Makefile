# Makefile for cexp examples


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


# -------- generic rules --------
# invoke the parser generator
%.gr.gen.cc %.gr.gen.h: %.gr
	$(ELKHOUND) -v -o $*.gr.gen $*.gr

# invoke the AST generator
%.ast.gen.cc %.ast.gen.h: %.ast
	$(AST)/astgen -b$*.ast.gen $*.ast

# compile a C++ source file
%.o: %.c
	g++ -c $(CCFLAGS) $(CPPFLAGS) $*.cc


# -------- targets --------
# main target
all: cexp3ast.ast.gen.h cexp3 cexp3b

# extra object files
extras := \
  $(PARSGEN)/cc/parssppt.o \
  $(PARSGEN)/cc/lexer2.o \
  $(PARSGEN)/cc/lexer1.o \
  $(PARSGEN)/cc/lexer1yy.o \
  $(PARSGEN)/cc/cc_lang.o \
  $(AST)/asthelp.o \
  $(AST)/strtable.o

# cexp3b: disambiguation using 'merge'
cexp3b.gr: cexp3.gr
	rm -f $@
	grep -v PREC cexp3.gr >$@
	chmod a-w $@

cexp3: $(PARSGEN)/glrmain.o cexp3.gr.gen.o cexp3mrg.o cexp3ast.ast.gen.o 
	g++ -g -o $@ $^ $(extras) $(LDFLAGS)
	./$@ $(PARSGEN)/in/cexp3.in1 | tee tmp.out
	grep 'result: 7' tmp.out
	rm tmp.out

cexp3b: $(PARSGEN)/glrmain.o cexp3b.gr.gen.o cexp3mrg.o cexp3ast.ast.gen.o
	g++ -g -o $@ $^ $(extras) $(LDFLAGS)
	./$@ $(PARSGEN)/in/cexp3.in1 | tee tmp.out
	grep 'result: 7' tmp.out
	rm tmp.out


clean:
	rm -f cexp3 cexp3b cexp3b.gr *.bin *.gen.* *.o
