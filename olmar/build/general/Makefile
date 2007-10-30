# Makefile for build/astgen
# see license.txt for copyright and terms of use

# set to ``cmo'' for ocaml bytecode compilation
# and to ``cmx'' for native compilation
OC_LIB_EXT := cmxa

# directories of other software
SMBASE := ../../../smbase
LIBSMBASE := $(SMBASE)/libsmbase.a
OCDIR := /usr/local/stow/ocaml-3.09-godi/lib/ocaml/std-lib


# external tools
OCC   := ocamlc.opt
OCOPT := ocamlopt.opt

# C++ compiler, etc.
CXX := ccache g++-4.1

# set to -native for native and empty for bytecode
OCDEPOPTS := -native

OCDEBUG=0
ifeq ($(OCDEBUG),1)
  OCOPT := $(OCC) -g
  OCCC  := $(OCC) -g
  OCC   := $(OCC) -g
  OC_OBJ_EXT := cmo
  OC_LIB_EXT := cma
  OCDEPOPTS :=
endif


# main targets
stage_1: ocaml_reflection_base.o astgen_general.$(OC_LIB_EXT)

stage_2:

stage_3:


# flags for the C and C++ compilers (and preprocessor)
CCFLAGS := -g -Wall -Wno-deprecated -D__UNIX__ -DNDEBUG -D__LINUX__ \
		-I$(SMBASE) -I $(OCDIR)

# list of files to clean in 'clean' (etc.) targets
# (these get added to below)
TOCLEAN =


# -include $(ASTGEN_OBJS:.o=.d)
-include ocaml_reflection_base.d


# compile .cc to .o
%.o: %.cc
	$(CXX) -c -o $@ $< $(CCFLAGS)
	@perl $(SMBASE)/depend.pl -o $@ $< $(CCFLAGS) > $*.d

TOCLEAN += *.cmo *.cmi *.cmx *.cmxa *.cma *.o *.a
%.cmi: %.mli
	$(OCC) -c $<

%.cmo: %.ml
	$(OCC) -c $<

%.cmx: %.ml
	$(OCOPT) -c $<

# remake the generated Makefile if its inputs have changed
#Makefile: Makefile.in config.status
#	./config.status

# reconfigure if the configure script has changed
#config.status: configure.pl $(SMBASE)/sm_config.pm
#	./config.status -reconfigure


# ------------------------ astgen ocaml lib ---------

ASTGENLIB_ML=\
	astgen_util.ml \
	ast_annotation.ml

astgen_general.cma: $(ASTGENLIB_ML:.ml=.cmo)
	$(OCC) -a -o $@ $^

astgen_general.cmxa: $(ASTGENLIB_ML:.ml=.cmx)
	$(OCOPT) -a -o $@ $^


-include mldeps.mk

.PHONY: mldeps
depend: mldeps
mldeps:
	ocamldep $(OCDEPOPTS) $(ASTGENLIB_ML) > mldeps.mk

# ------------------------ misc ---------------------

# delete outputs of compiler, linker
clean:
	rm -f $(TOCLEAN)
	rm -f *.o tmp *.d gmon.out
	rm -f agrampar astgen ccsstr ocsstr towner exampletest libast.a
	rm -f agrampar.output
	rm -f example.{h,cc} ext1.{h,cc}
	rm -f agramlex.yy.cc

# return to pristine checked-out state
distclean: clean
	rm -f Makefile config.status config.summary
	rm -rf gendoc

