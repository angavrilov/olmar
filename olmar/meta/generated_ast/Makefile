# Makefile for olmar/meta/generated_ast
#
#  Copyright 2006-2007 Hendrik Tews, All rights reserved. #
#  See file license.txt for terms of use                  #
###########################################################


# other directories
GENERAL:=  ../../build/general
ASTGEN_OR:= ../../build/astgen


# external tools
OCAMLC   := ocamlc.opt
OCAMLOPT := ocamlopt.opt
OCAMLCC  := ocamlopt.opt
CAMLP4O  := camlp4o

OCAML_OBJ_EXT := cmx
OCAML_LIB_EXT := cmxa
OCAML_FLAGS := -w Ae
OCAML_BYTE_FLAGS := -g
OCAMLDEPOPTS := -native

OCAMLDEBUG=0
ifeq ($(OCAMLDEBUG),1)
  OCAMLOPT := $(OCAMLC) -g
  OCAMLCC  := $(OCAMLC) -g
  OCAMLC   := $(OCAMLC) -g
  OCAML_OBJ_EXT := cmo
  OCAML_LIB_EXT := cma
  OCAMLDEPOPTS :=
endif

.PHONY: all
all: 	superast.cmx


# list of files to clean in 'clean' (etc.) targets
# (these get added to below)
TOCLEAN =
TODISTCLEAN =

# # re-create the Makefile if Makefile.in has changed
# TODISTCLEAN += Makefile
# Makefile: Makefile.in config.status
# 	./config.status

# # reconfigure if the configure script has changed
# config.status: configure.pl $(SMBASE)/sm_config.pm
# 	./config.status -reconfigure


############################################################################
#### accessors
#### 

TOCLEAN += ast_accessors_generated.ml
ast_accessors_generated.ml: \
		../gen_accessors $(ASTGEN_OR)/ast.ast.oast
	../gen_accessors $(ASTGEN_OR)/ast.ast.oast \
		-tr ../ast_reflection_control.txt \
		-o ast_accessors_generated

TOCLEAN += ast_accessors.ml
ast_accessors.ml: ast_accessors_header.ml ast_accessors_generated.ml \
		ast_accessors_trailer.ml
	echo "# 1 \"`pwd`/ast_accessors_header.ml\"" > $@
	cat ast_accessors_header.ml >> $@
	echo "# 1 \"`pwd`/ast_accessors_generated.ml\"" >> $@
	cat ast_accessors_generated.ml >> $@
	echo "# 1 \"`pwd`/ast_accessors_trailer.ml\"" >> $@
	cat ast_accessors_trailer.ml >> $@


############################################################################
#### superast
#### 

TOCLEAN += superast_generated.ml superast_generated.mli
superast_generated.ml superast_generated.mli: \
		../gen_superast $(ASTGEN_OR)/ast.ast.oast
	../gen_superast $(ASTGEN_OR)/ast.ast.oast \
		-tr ../ast_reflection_control.txt \
		-o superast_generated

TOCLEAN += superast.mli
superast.mli: superast_header.mli superast_generated.mli superast_trailer.mli
	echo "# 1 \"`pwd`/superast_header.mli\"" > $@
	cat superast_header.mli >> $@
	echo "# 1 \"`pwd`/superast_generated.mli\"" >> $@
	cat superast_generated.mli >> $@
	echo "# 1 \"`pwd`/superast_trailer.mli\"" >> $@
	cat superast_trailer.mli >> $@

TOCLEAN += superast.ml
superast.ml: superast_header.ml superast_generated.ml superast_trailer.ml
	echo "# 1 \"`pwd`/superast_header.ml\"" > $@
	cat superast_header.ml >> $@
	echo "# 1 \"`pwd`/superast_generated.ml\"" >> $@
	cat superast_generated.ml >> $@
	echo "# 1 \"`pwd`/superast_trailer.ml\"" >> $@
	cat superast_trailer.ml >> $@


superast.cmi: ast_accessors.$(OCAML_OBJ_EXT)
superast.cmx: superast.cmi
superast.cmo: superast.cmi

############################################################################
#### Misc
#### 

.PHONY: clean
clean:
	rm -f $(TOCLEAN)

.PHONY: distclean
distclean: clean
	rm -f Makefile config.status config.summary
	rm -f astmapfuns.mli


TOCLEAN += *.cmo *.cmxa *.cma *.a
%.cmo: %.ml
	$(OCAMLC) $(OCAML_BYTE_FLAGS) $(OCAML_FLAGS) -I $(GENERAL) \
		-I $(ASTGEN_OR) -I .. -c $<

TOCLEAN += *.cmx *.o
%.cmx: %.ml
	$(OCAMLOPT) $(OCAML_FLAGS) -I $(GENERAL) -I $(ASTGEN_OR) -I .. -c $<

TOCLEAN += *.cmi
%.cmi: %.mli
	$(OCAMLC) $(OCAML_FLAGS) -I $(GENERAL) -I $(ASTGEN_OR) -I .. -c $<


-include mldeps.mk

.PHONY: mldeps
depend: mldeps
mldeps:
	ocamldep $(OCAMLDEPOPTS) -I $(GENERAL) -I $(ASTGEN_OR) \
		*ml *mli > mldeps.mk
