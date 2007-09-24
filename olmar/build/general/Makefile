# Makefile for build/astgen
# see license.txt for copyright and terms of use

# set to ``cmo'' for ocaml bytecode compilation
# and to ``cmx'' for native compilation
OCAML_LIB_EXT := cmxa

# main targets
all: ocaml_reflection_base.o astgen_general.$(OCAML_LIB_EXT)


# directories of other software
SMBASE := ../../../smbase
LIBSMBASE := $(SMBASE)/libsmbase.a
OCAMLDIR := /usr/local/stow/ocaml-3.09.3/lib/ocaml


# external tools
OCAMLC   := ocamlc.opt
OCAMLOPT := ocamlopt.opt

# C++ compiler, etc.
CXX := ccache g++-4.1

# set to -native for native and empty for bytecode
OCAMLDEPOPTS := -native

# flags for the C and C++ compilers (and preprocessor)
CCFLAGS := -g -Wall -Wno-deprecated -D__UNIX__ -DNDEBUG -D__LINUX__ \
		-I$(SMBASE) -I $(OCAMLDIR)

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
	$(OCAMLC) -c $<

%.cmo: %.ml
	$(OCAMLC) -c $<

%.cmx: %.ml
	$(OCAMLOPT) -c $<

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
	$(OCAMLC) -a -o $@ $^

astgen_general.cmxa: $(ASTGENLIB_ML:.ml=.cmx)
	$(OCAMLOPT) -a -o $@ $^


-include mldeps.mk

.PHONY: mldeps
depend: mldeps
mldeps:
	ocamldep $(OCAMLDEPOPTS) $(ASTGENLIB_ML) > mldeps.mk

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

