# Makefile for olmar/meta
#
#  Copyright 2006-2007 Hendrik Tews, All rights reserved. #
#  See file license.txt for terms of use                  #
###########################################################

# Makefile.in
# this is the Makefile for the ocaml ast tools

# use the first goal for development, for release put it in comment and use all

first: stage_1 ast.type.dot ast.ast.dot
#first: gen_reflection
#first: ast_reflection diff_ast_reflection
#first: check_oast

MEMCHECK:= $(HOME)/src/memcheck
UTIL:= ../util
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

.PHONY: stage_1
stage_1: check_oast ast_type_graph gen_reflection gen_superast gen_accessors \
		gen_uplinks gen_graph 
	$(MAKE) -C generated_ast all

stage_2:


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
#### check_oast
#### 

CHECKTREE_ML=\
	$(MEMCHECK)/memcheck.ml \
	$(GENERAL)/astgen_general.$(OCAML_LIB_EXT) \
	$(ASTGEN_OR)/astgen_lib.$(OCAML_LIB_EXT) \
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


#############################################################################
#####
##### gen_reflection


GEN_REFLECTION_ML=\
	$(UTIL)/olmar_utils.$(OCAML_LIB_EXT) \
	$(GENERAL)/astgen_general.$(OCAML_LIB_EXT) \
	$(ASTGEN_OR)/astgen_lib.$(OCAML_LIB_EXT) \
	ast_config.ml \
	meta_ast.ml \
	gen_reflection.ml

TOCLEAN += gen_reflection
gen_reflection: $(GEN_REFLECTION_ML:.ml=.$(OCAML_OBJ_EXT))
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ $^


.PHONY: ast_reflection
ast_reflection: gen_reflection
	rm -rf $(ASTGEN_OR)/new
	mkdir $(ASTGEN_OR)/new
	./gen_reflection $(ASTGEN_OR)/ast.ast.oast \
		-tr ast_reflection_control.txt \
		-o $(ASTGEN_OR)/new/ast_reflect

.PHONY: diff_ast_reflection
diff_ast_reflection:
	$(MAKE) -C $(ASTGEN) diff_reflection


#############################################################################
#####
##### gen_superast

GEN_SUPERAST_ML=\
	$(UTIL)/olmar_utils.$(OCAML_LIB_EXT) \
	$(GENERAL)/astgen_general.$(OCAML_LIB_EXT) \
	$(ASTGEN_OR)/astgen_lib.$(OCAML_LIB_EXT) \
	ast_config.ml \
	meta_ast.ml \
	gen_superast.ml


TOCLEAN += gen_superast
gen_superast: $(GEN_SUPERAST_ML:.ml=.$(OCAML_OBJ_EXT))
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ $^


#############################################################################
#####
##### gen_accessors

GEN_ACCESSORS_ML=\
	$(UTIL)/olmar_utils.$(OCAML_LIB_EXT) \
	$(GENERAL)/astgen_general.$(OCAML_LIB_EXT) \
	$(ASTGEN_OR)/astgen_lib.$(OCAML_LIB_EXT) \
	ast_config.ml \
	meta_ast.ml \
	gen_accessors.ml


TOCLEAN += gen_accessors
gen_accessors: $(GEN_ACCESSORS_ML:.ml=.$(OCAML_OBJ_EXT))
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ $^


#############################################################################
#####
##### gen_uplinks

GEN_UPLINKS_ML=\
	$(UTIL)/olmar_utils.$(OCAML_LIB_EXT) \
	$(GENERAL)/astgen_general.$(OCAML_LIB_EXT) \
	$(ASTGEN_OR)/astgen_lib.$(OCAML_LIB_EXT) \
	ast_config.ml \
	meta_ast.ml \
	gen_uplinks.ml


TOCLEAN += gen_uplinks
gen_uplinks: $(GEN_UPLINKS_ML:.ml=.$(OCAML_OBJ_EXT))
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ $^


#############################################################################
#####
##### gen_graph

GEN_GRAPH_ML=\
	$(UTIL)/olmar_utils.$(OCAML_LIB_EXT) \
	$(GENERAL)/astgen_general.$(OCAML_LIB_EXT) \
	$(ASTGEN_OR)/astgen_lib.$(OCAML_LIB_EXT) \
	ast_config.ml \
	meta_ast.ml \
	gen_graph.ml


TOCLEAN += gen_graph
gen_graph: $(GEN_GRAPH_ML:.ml=.$(OCAML_OBJ_EXT))
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ $^

TOCLEAN += ast.ast.dot
ast.ast.dot: $(ASTGEN_OR)/ast.ast.oast generated_ast/ast_graph \
		ast_reflection_control.txt
	generated_ast/ast_graph -tr ast_reflection_control.txt \
		-o ast.ast.dot $(ASTGEN_OR)/ast.ast.oast 


#############################################################################
#####
##### ast_type_graph

# $(UTIL)/olmar_utils.$(OCAML_LIB_EXT) \
# $(GENERAL)/astgen_general.$(OCAML_LIB_EXT) \
# $(ASTGEN_OR)/astgen_lib.$(OCAML_LIB_EXT) \
# ast_config.ml \
# meta_ast.ml \

AST_TYPE_GRAPH_ML=\
	$(UTIL)/olmar_utils.$(OCAML_LIB_EXT) \
	$(GENERAL)/astgen_general.$(OCAML_LIB_EXT) \
	$(ASTGEN_OR)/astgen_lib.$(OCAML_LIB_EXT) \
	ast_config.ml \
	meta_ast.ml \
	ast_type_graph.ml


TOCLEAN += ast_type_graph
ast_type_graph: $(AST_TYPE_GRAPH_ML:.ml=.$(OCAML_OBJ_EXT))
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ $^

TOCLEAN += ast.type.dot
ast.type.dot: $(ASTGEN_OR)/ast.ast.oast ast_type_graph \
		ast_reflection_control.txt
	./ast_type_graph -tr ast_reflection_control.txt \
		-o ast.type.dot $(ASTGEN_OR)/ast.ast.oast 


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
	$(OCAMLC) $(OCAML_BYTE_FLAGS) $(OCAML_FLAGS) -I $(UTIL) -I $(GENERAL) \
		-I $(ASTGEN_OR) -c $<

TOCLEAN += *.cmx *.o
%.cmx: %.ml
	$(OCAMLOPT) $(OCAML_FLAGS) -I $(UTIL) -I $(GENERAL) -I $(ASTGEN_OR) \
		-c $<

TOCLEAN += *.cmi
%.cmi: %.mli
	$(OCAMLC) $(OCAML_FLAGS) -I $(UTIL) -I $(GENERAL) -I $(ASTGEN_OR) -c $<


-include mldeps.mk

.PHONY: mldeps
depend: mldeps
mldeps:
	ocamldep $(OCAMLDEPOPTS) -I $(UTIL) -I $(GENERAL) -I $(ASTGEN_OR) \
		*ml *mli > mldeps.mk
