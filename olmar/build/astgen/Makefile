# Makefile for build/astgen
# see license.txt for copyright and terms of use

# set to 1 for native compilation, 0 for bytecode
NATIVE_OCAML := 1
# set to ``cmo'' for ocaml bytecode compilation
# and to ``cmx'' for native compilation
OCAML_OBJ_EXT := cmx
OCAML_LIB_EXT := cmxa


# main targets
stage_1: astgen_lib.$(OCAML_LIB_EXT) astgen_or ast.ast.oast

stage_2: 

stage_3: check_ast_oast


# directories of other software
SMBASE := ../../../smbase
AST := ../../../ast
GENERAL := ../general
LIBSMBASE := $(SMBASE)/libsmbase.a
OCAMLDIR := /usr/local/stow/ocaml-3.09-godi/lib/ocaml/std-lib
META := ../../meta

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

# C++ compiler, etc.
CXX := ccache g++-4.1

# flags for the C and C++ compilers (and preprocessor)
CCFLAGS := -g -Wall -Wno-deprecated -D__UNIX__ -DNDEBUG -D__LINUX__ \
		-I$(SMBASE) -I $(AST) -I $(GENERAL) -I $(OCAMLDIR)

# flags for the linker
LDFLAGS := -g -Wall $(LIBSMBASE)


# list of files to clean in 'clean' (etc.) targets
# (these get added to below)
TOCLEAN =

# compile .cc to .o
%.o: %.cc
	$(CXX) -c -o $@ $< $(CCFLAGS)
	@perl $(SMBASE)/depend.pl -o $@ $< $(CCFLAGS) > $*.d

TOCLEAN += *.cmo *.cmi *.cmx *.cmxa *.cma *.o *.a
%.cmi: %.mli
	$(OCAMLC) -I $(GENERAL) -c $<

%.cmo: %.ml
	$(OCAMLC) -I $(GENERAL) -c $<

%.cmx: %.ml
	$(OCAMLOPT) -I $(GENERAL) -c $<

# remake the generated Makefile if its inputs have changed
#Makefile: Makefile.in config.status
#	./config.status

# reconfigure if the configure script has changed
#config.status: configure.pl $(SMBASE)/sm_config.pm
#	./config.status -reconfigure


#############################################################################
#####
#####  astgen_or
#####
#############################################################################


TOCLEAN += ast_oast_header.ml
ast_oast_header.ml: ast_oast_header_start.ml \
		$(GENERAL)/oast_header_trailer.ml $(AST)/ast.ast
	rm -f $@
	cat ast_oast_header_start.ml >> $@
	head -1 $(AST)/ast.ast | tr "." " " | \
		(read -a aa; echo -n $${aa[5]}) >> $@
	echo -e '\n#1' "\"$(realpath $(GENERAL)/oast_header_trailer.ml)\"" >> $@
	cat $(GENERAL)/oast_header_trailer.ml >> $@


# List of ml modules needed by astgen. They are all combined 
# into one ml_object.o file.

ASTGEN_ML := \
	ast_ml_types.ml \
	ast_ml_constructors.ml \
	ast_reflect_type.ml \
	ast_oast_header.ml

-include mldeps.mk

.PHONY: mldeps
depend: mldeps
mldeps:
	ocamldep $(OCAMLDEPOPTS) -I $(GENERAL) $(ASTGEN_ML) > mldeps.mk


astgen_lib.cmxa : $(ASTGEN_ML:.ml=.cmx)
	$(OCAMLOPT) -o $@ -a $^

astgen_lib.cma : $(ASTGEN_ML:.ml=.cmo)
	$(OCAMLC) -o $@ -a $^


ASTGEN_ML_OBJECT := \
	$(GENERAL)/astgen_general.$(OCAML_LIB_EXT) \
	astgen_lib.$(OCAML_LIB_EXT) \
	ast_caml_callbacks.ml


astgen_ml_object.o: $(ASTGEN_ML_OBJECT:.ml=.$(OCAML_OBJ_EXT)) 
	$(OCAMLCC) -output-obj -o $@ $^	


.PHONY: astgen_patch
astgen_patch:
	diff -U 10 $(AST)/astgen.cc astgen_or.cc > astgen_reflection.patch; true

TOCLEAN += astgen_or.cc
astgen_or.cc: $(AST)/astgen.cc astgen_reflection.patch
	patch -i astgen_reflection.patch -o astgen_or.cc $(AST)/astgen.cc

# list here those objects that are not libast
ASTGEN_OR_OBJS := \
  $(AST)/agrampar.tab.o \
  $(AST)/agramlex.yy.o \
  $(AST)/ast.hand.o \
  $(AST)/agrampar.o  \
  $(GENERAL)/ocaml_reflection_base.o \
  ast_reflect.o \
  ast_reflect_ocaml_reflect.o \
  astgen_ml_object.o \
  astgen_or.o
-include $(ASTGEN_OR_OBJS:.o=.d)


# ast.ast.cc is a dependency here but not explicitly in the command
# line because ast.hand.cc #includes it
TOCLEAN += astgen_or
astgen_or: $(ASTGEN_OR_OBJS) $(LIBSMBASE) 
	$(CXX) -o astgen_or $(ASTGEN_OR_OBJS) $(LDFLAGS) $(AST)/libast.a \
		$(OCAMLLIBS)


#############################################################################
#####
#####  astgen_or test
#####
#############################################################################


TOCLEAN += ast.ast.oast ast.ast.h ast.ast.cc
ast.ast.h ast.ast.cc ast.ast.oast: astgen_or $(AST)/ast.ast
	./astgen_or -oast.ast $(AST)/ast.ast

.PHONY: diff-ast-ast
diff-ast-ast: ast.ast.h ast.ast.cc
	echo ===================== ast.ast.h =================================
	diff $(AST)/ast.ast.h ast.ast.h; true
	echo ===================== ast.ast.cc ================================
	diff $(AST)/ast.ast.cc ast.ast.cc; true


#############################################################################
#####
#####  check ast oast
#####
#############################################################################


.PHONY: check_ast_oast
check_ast_oast: ast.ast.oast $(META)/check_oast
	$(META)/check_oast -ast ast.ast.oast


#############################################################################
#####
#####  bootstrap new astgen version
#####
#############################################################################


# Don't change the original ast.ast for bootstrapping! Via 
# ast_oast_header almost everything depends on ast.ast and will get 
# recompiled. Change the local copy new-ast.ast instead!
#
#  1. make bootstrap-prepare
#  2. edit new-ast.ast
#  3. make diff-new-ast
#  4. make bootstrap-ast-ast
#  5. make diff-ast-ast
#  6. make update-reflection
#  7. make diff-reflection
#  8. make astgen-orig
#  9. make generation
# 10. make astgen_or
# 11. repeat 4, 5
# 12. make new-reflection
# 13. repeat 6, 7

.PHONY: bootstrap-prepare
bootstrap-prepare: astgen_or $(META)/gen_reflection
	d=bootstrap-$$(date +%Y-%m-%d-%H-%M-%S); \
	mkdir $$d; \
	cp $(AST)/ast.ast $(AST)/ast.ast.h $(AST)/ast.ast.cc \
		./astgen_or $(META)/gen_reflection $$d
	cp $(AST)/ast.ast new-ast.ast

.PHONY: diff-new-ast
diff-new-ast:
	diff $(AST)/ast.ast new-ast.ast

.PHONY: bootstrap-ast-ast
bootstrap-ast-ast:
	./astgen_or -oast.ast new-ast.ast

.PHONY: update-reflection
update-reflection: $(META)/gen_reflection
	$(MAKE) -C $(META) ast_reflection

.PHONY: diff-reflection
diff-reflection:
	for f in new/* ; do \
	    echo ======== $$(basename $$f) ==================================; \
	    diff $$f $$(basename $$f); \
	    true; \
	done

.PHONY: astgen-orig
astgen-orig:
	cp new-ast.ast $(AST)/ast.ast
	cd $(AST); ./astgen -oast.ast ast.ast
	$(MAKE) -C $(AST)

.PHONY: new-reflection
new-reflection:
	$(MAKE) -C $(META) gen_reflection



#############################################################################
#####
#####  Misc
#####
#############################################################################


# compile with stuff in new
generation:
	rm -rf old
	mkdir old
	mv ast_reflect_ocaml_reflect.cc ast_reflect_ocaml_reflect.h \
		ast_reflect_type.ml ast.ast.oast old
	mv new/* .
	make clean

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

