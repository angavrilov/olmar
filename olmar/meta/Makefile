# Makefile for olmar/meta
#
#  Copyright 2006-2007 Hendrik Tews, All rights reserved. #
#  See file license.txt for terms of use                  #
###########################################################

# Makefile.in
# this is the Makefile for the ocaml ast tools

# use the first goal for development, for release put it in comment and use all

first: gen_reflection do_generated
#first: do_reflection diff_reflection
#first: check_oast

MEMCHECK:= $(HOME)/src/memcheck
GENERAL:=  ../build/general
ASTGEN_OR:= ../build/astgen

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
all: 	check_oast gen_reflection gen_superast gen_accessors gen_uplinks\
		dense_set.$(OCAML_OBJ_EXT)
	$(MAKE) -C generated_ast all



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


#############################################################################
#####
##### gen_uplinks

GEN_UPLINKS_ML=\
	$(GENERAL)/ast_annotation.ml \
	$(ASTGEN_OR)/ast_oast_header.ml \
	more_string.ml \
	ast_config.ml \
	meta_ast.ml \
	gen_uplinks.ml


TOCLEAN += gen_uplinks
gen_uplinks: $(GEN_UPLINKS_ML:.ml=.$(OCAML_OBJ_EXT))
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ $^


#############################################################################
#####
##### gen_accessors

GEN_ACCESSORS_ML=\
	$(GENERAL)/ast_annotation.ml \
	$(ASTGEN_OR)/ast_oast_header.ml \
	more_string.ml \
	ast_config.ml \
	meta_ast.ml \
	gen_accessors.ml


TOCLEAN += gen_accessors
gen_accessors: $(GEN_ACCESSORS_ML:.ml=.$(OCAML_OBJ_EXT))
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ $^


#############################################################################
#####
##### gen_superast

GEN_SUPERAST_ML=\
	$(GENERAL)/ast_annotation.ml \
	$(ASTGEN_OR)/ast_oast_header.ml \
	more_string.ml \
	ast_config.ml \
	meta_ast.ml \
	gen_superast.ml


TOCLEAN += gen_superast
gen_superast: $(GEN_SUPERAST_ML:.ml=.$(OCAML_OBJ_EXT))
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ $^


.PHONY: do_generated
do_generated: gen_superast dense_set.$(OCAML_OBJ_EXT) gen_accessors gen_uplinks
	$(MAKE) -C generated_ast

#############################################################################
#####
##### gen_reflection


GEN_REFLECTION_ML=\
	$(GENERAL)/astgen_general.$(OCAML_LIB_EXT) \
	$(ASTGEN_OR)/ast_oast_header.ml \
	more_string.ml \
	ast_config.ml \
	meta_ast.ml \
	gen_reflection.ml

TOCLEAN += gen_reflection
gen_reflection: $(GEN_REFLECTION_ML:.ml=.$(OCAML_OBJ_EXT))
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ $^


.PHONY: do_reflection
do_reflection: gen_reflection
	rm -rf $(ASTGEN_OR)/new
	mkdir $(ASTGEN_OR)/new
	./gen_reflection $(ASTGEN_OR)/ast.ast.oast \
		-tr ast_reflection_control.txt \
		-o $(ASTGEN_OR)/new/ast_reflect

.PHONY: diff_reflection
diff_reflection:
	for f in $(ASTGEN_OR)/new/* ; do \
	    echo ======== $$(basename $$f) ==================================; \
	    diff $$f $(ASTGEN_OR)/$$(basename $$f); \
	    true; \
	done


############################################################################
#### memcheck type def generation
#### 
#### for astgen asts

AST_AST_TYPE_DEF=\
	$(GENERAL)/ast_annotation.ml \
	$(ASTGEN_OR)/ast_ml_types.ml \
	$(ASTGEN_OR)/ast_reflect_type.ml

AST_AST_TYPE_DESCR=\
	ast_type_descr_header.ml \
	$(MEMCHECK)/generate_type_descr.cmo

TOCLEAN += ast_ast_type_descr.ml
ast_ast_type_descr.ml: $(AST_AST_TYPE_DEF) $(AST_AST_TYPE_DESCR) Makefile
	cat ast_type_descr_header.ml > $@
	for f in $(AST_AST_TYPE_DEF) ; do \
		$(CAMLP4O) $(MEMCHECK)/generate_type_descr.cmo $$f >> $@; \
	done

ast_ast_type_descr.cmo: ast_ast_type_descr.ml $(MEMCHECK)/memcheck.cmi
	$(OCAMLC) $(OCAML_BYTE_FLAGS) $(OCAML_FLAGS) -I $(MEMCHECK) -c $<

ast_ast_type_descr.cmx: ast_ast_type_descr.ml $(MEMCHECK)/memcheck.cmx
	$(OCAMLOPT) $(OCAML_FLAGS) -I $(MEMCHECK) -c $<


#### for elsa asts

## ELSA_AST_TYPE_DEF=\
## 	/usr/local/lib/ocaml/hashtbl.ml \
## 	$(ELSA)/cc_ml_types.ml \
## 	$(ELSA)/ml_ctype.ml \
## 	$(AST)/ast_annotation.ml \
## 	$(ELSA)/cc_ast_gen_type.ml
## 
## ELSA_AST_TYPE_DESCR=\
## 	ast_type_descr_header.ml\
## 	$(MEMCHECK)/generate_type_descr.cmo
## 
## TOCLEAN += elsa_ast_type_descr.ml
## elsa_ast_type_descr.ml: $(ELSA_AST_TYPE_DEF) $(ELSA_AST_TYPE_DESCR) Makefile
## 	cat ast_type_descr_header.ml > $@
## 	for f in $(ELSA_AST_TYPE_DEF) ; do \
## 		$(CAMLP4O) $(MEMCHECK)/generate_type_descr.cmo $$f >> $@; \
## 	done
## 
## elsa_ast_type_descr.cmo: elsa_ast_type_descr.ml $(MEMCHECK)/memcheck.cmi
## 	$(OCAMLC) $(OCAML_BYTE_FLAGS) $(OCAML_FLAGS) -I $(MEMCHECK) -c $<
## 
## elsa_ast_type_descr.cmx: elsa_ast_type_descr.ml $(MEMCHECK)/memcheck.cmx
## 	$(OCAMLOPT) $(OCAML_FLAGS) -I $(MEMCHECK) -c $<


############################################################################
#### check_oast
#### 

CHECKTREE_ML=\
	$(MEMCHECK)/memcheck.ml \
	$(GENERAL)/ast_annotation.ml \
	$(ASTGEN_OR)/ast_oast_header.ml \
	ast_ast_type_descr.ml \
	check_oast.ml

check_oast.cmo: check_oast.ml ast_ast_type_descr.cmo
	$(OCAMLC) $(OCAML_BYTE_FLAGS) $(OCAML_FLAGS) \
		-I $(MEMCHECK) -I $(GENERAL) -I $(ASTGEN_OR) -c $<

check_oast.cmx: check_oast.ml ast_ast_type_descr.cmx
	$(OCAMLOPT) $(OCAML_FLAGS) \
		-I $(MEMCHECK) -I $(GENERAL) -I $(ASTGEN_OR) -c $<

TOCLEAN += check_oast
check_oast: $(CHECKTREE_ML:.ml=.$(OCAML_OBJ_EXT))
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ -I $(MEMCHECK) -I $(GENERAL) \
		-I $(ASTGEN_OR) unix.$(OCAML_LIB_EXT) $^

TOCLEAN += check_oast_byte
check_oast_byte: $(CHECKTREE_ML:.ml=.cmo)
	$(OCAMLC) $(OCAML_BYTE_FLAGS) $(OCAML_FLAGS) -o $@ \
		-I $(MEMCHECK) -I $(GENERAL) -I $(ASTGEN_OR) unix.cma $^


$(MEMCHECK)/generate_type_descr.cmo:
	$(MAKE) -C $(MEMCHECK) all

$(MEMCHECK)/memcheck.cmo:
	$(MAKE) -C $(MEMCHECK) all

$(MEMCHECK)/memcheck.cmx:
	$(MAKE) -C $(MEMCHECK) all


############################################################################
#### Misc
#### 

.PHONY: clean
clean:
	rm -f $(TOCLEAN)
	$(MAKE) -C generated_ast clean

.PHONY: distclean
distclean: clean
	rm -f Makefile config.status config.summary
	rm -f astmapfuns.mli


TOCLEAN += *.cmo *.cmxa *.cma *.a
%.cmo: %.ml
	$(OCAMLC) $(OCAML_BYTE_FLAGS) $(OCAML_FLAGS) -I $(GENERAL) \
		-I $(ASTGEN_OR) -c $<

TOCLEAN += *.cmx *.o
%.cmx: %.ml
	$(OCAMLOPT) $(OCAML_FLAGS) -I $(GENERAL) -I $(ASTGEN_OR) -c $<

TOCLEAN += *.cmi
%.cmi: %.mli
	$(OCAMLC) $(OCAML_FLAGS) -I $(GENERAL) -I $(ASTGEN_OR) -c $<


-include mldeps.mk

.PHONY: mldeps
depend: mldeps
mldeps:
	ocamldep $(OCAMLDEPOPTS) -I $(GENERAL) -I $(ASTGEN_OR) \
		*ml *mli > mldeps.mk
