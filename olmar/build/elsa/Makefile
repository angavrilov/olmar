# Makefile for build/elsa
# see license.txt for copyright and terms of use

# set to 1 for native compilation, 0 for bytecode
NATIVE_OCAML := 1
# set to ``cmo'' for ocaml bytecode compilation
# and to ``cmx'' for native compilation
OCAML_OBJ_EXT := cmx
OCAML_LIB_EXT := cmxa


# main targets
first: elsa.type.dot
stage_1:
stage_2: elsa.ast.oast

#all: astgen_lib.$(OCAML_LIB_EXT) astgen_or ast.ast.oast

# directories of other software
SMBASE := ../../../smbase
AST := ../../../ast
ELSA := ../../../elsa
GENERAL := ../general
ASTGEN := ../astgen
META := ../../meta

LIBSMBASE := $(SMBASE)/libsmbase.a
OCAMLDIR := /usr/local/stow/ocaml-3.09-godi/lib/ocaml/std-lib

ifeq ($(NATIVE_OCAML),1)
#native
OCAMLLIBS := -L $(OCAMLDIR) -lasmrun -ldl
OCAMLDEPOPTS := -native
else
#bytecode
OCAMLLIBS := -L $(OCAMLDIR) -lcamlrun -lcurses -ldl
OCAMLDEPOPTS :=
endif



# external tools
PERL := perl
OCAMLC   := ocamlc.opt
OCAMLCC  := ocamlopt.opt
OCAMLOPT := ocamlopt.opt
DEP := $(PERL) $(SMBASE)/depend.pl


# list of files to clean in 'clean' (etc.) targets
# (these get added to below)
TOCLEAN =

# # re-create the Makefile if Makefile.in has changed
# TODISTCLEAN += Makefile
# Makefile: Makefile.in config.status
# 	./config.status

# # reconfigure if the configure script has changed
# config.status: configure.pl $(SMBASE)/sm_config.pm
# 	./config.status -reconfigure


# ----------------------- compiler configuration -------------------
# C++ preprocessor, compiler and linker
CXX := ccache g++-4.1

# flags for the C++ compiler (and preprocessor)
CCFLAGS := -g -Wall -Wno-deprecated -D__UNIX__ -DNDEBUG -D__LINUX__ -Woverloaded-virtual -I$(SMBASE) -I$(AST) -I$(ELKHOUND) -I$(OCAMLDIR)

# flags for the linker
libraries := $(LIBELKHOUND) $(LIBAST) $(LIBSMBASE)
LDFLAGS := -g -Wall $(libraries)


# compile .cc in this directory to a .o
TOCLEAN += *.o
TODEPCLEAN += *.d
%.o: %.cc
	$(CXX) -c -o $@ $< $(if $(findstring $*,$(GCOV_MODS)),$(GCOV_OPTS) )$(CCFLAGS)
	@$(DEP)   -o $@ $< $(if $(findstring $*,$(GCOV_MODS)),$(GCOV_OPTS) )$(CCFLAGS) >$*.d

TOCLEAN += *.cmo *.cmi *.cmx
%.cmo: %.ml
	$(OCAMLC) -c -I $(AST) $<

%.cmi: %.mli
	$(OCAMLC) -c -I $(AST) $<

%.cmx: %.ml
	$(OCAMLOPT) -c -I $(AST) $<


# ----------------------- elsa oast generation -------------------

.PHONY: cc_ast_patch
cc_ast_patch:
	diff -U 10 $(ELSA)/cc.ast cc.ast > cc.ast.patch; true

TOCLEAN += cc.ast
cc.ast: $(ELSA)/cc.ast cc.ast.patch
	patch -i cc.ast.patch -o cc.ast $(ELSA)/cc.ast



ELSA_AST_FILES :=\
	cc.ast \
	$(ELSA)/cc_tcheck.ast \
	$(ELSA)/cc_print.ast \
	$(ELSA)/cfg.ast \
	$(ELSA)/cc_elaborate.ast \
	$(ELSA)/gnu.ast \
	$(ELSA)/kandr.ast \
	./cc_type.ast


# run astgen_or to get the elsa.ast syntaxtree in ocaml
ELSA_AST_GEN_FILES := elsa.ast.oast
TOCLEAN += elsa.ast.oast
elsa.ast.oast: $(ELSA_AST_FILES) $(ASTGEN)/astgen_or
	$(ASTGEN)/astgen_or -oelsa.ast $(ELSA_AST_FILES)
	rm elsa.ast.h elsa.ast.cc


.PHONY: check_elsa_oast
check_elsa_oast: elsa.ast.oast $(META)/check_oast
	$(META)/check_oast -ast elsa.ast.oast


elsa.type.dot: elsa.ast.oast elsa_reflection_control.txt $(META)/ast_type_graph
	$(META)/ast_type_graph -o elsa.type.dot \
		-tr elsa_reflection_control.txt elsa.ast.oast


# .PHONY: elsa_reflection
# elsa_reflection: gen_reflection
# 	rm -rf $(ELSA_OR)/new
# 	mkdir $(ELSA_OR)/new
# 	./gen_reflection $(ELSA_OR)/elsa.ast.oast \
# 		-tr elsa_reflection_control.txt \
# 		-o $(ELSA_OR)/new/elsa_reflect




# ------------------------ Misc ---------------------------------

clean:
	rm -f $(TOCLEAN)
