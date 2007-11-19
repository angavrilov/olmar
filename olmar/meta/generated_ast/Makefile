# Makefile for olmar/meta/generated_ast
#
#  Copyright 2006-2007 Hendrik Tews, All rights reserved. #
#  See file license.txt for terms of use                  #
###########################################################


# other directories
UTIL:= ../../util
GENERAL:=  ../../build/general
ASTGEN_OR:= ../../build/astgen


# external tools
OCC   := ocamlc.opt
OCOPT := ocamlopt.opt
OCCC  := ocamlopt.opt
CAMLP4O  := camlp4o

OC_OBJ_EXT := cmx
OC_LIB_EXT := cmxa
OC_FLAGS := -w Ae
OC_BYTE_FLAGS := -g
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

.PHONY: all
all: 	superast.$(OC_OBJ_EXT) uplinks.$(OC_OBJ_EXT)


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
#### ast_graph
#### 

TOCLEAN += ast_graph_generated.ml
ast_graph_generated.ml: ../gen_graph ../ast_reflection_control.txt \
		$(ASTGEN_OR)/ast.ast.oast
	../gen_graph $(ASTGEN_OR)/ast.ast.oast \
		-tr ../ast_reflection_control.txt \
		-o ast_graph_generated

TOCLEAN += ast_graph.ml
ast_graph.ml: ast_graph_header.ml ast_graph_generated.ml \
		ast_graph_trailer.ml
	echo "# 1 \"`pwd`/ast_graph_header.ml\"" > $@
	cat ast_graph_header.ml >> $@
	echo "# 1 \"`pwd`/ast_graph_generated.ml\"" >> $@
	cat ast_graph_generated.ml >> $@
	echo "# 1 \"`pwd`/ast_graph_trailer.ml\"" >> $@
	cat ast_graph_trailer.ml >> $@

ast_graph.cmx: uplinks.cmx
ast_graph.cmo: uplinks.cmo

AST_GRAPH_ML=\
	$(UTIL)/olmar_utils.$(OC_LIB_EXT) \
	$(GENERAL)/astgen_general.$(OC_LIB_EXT) \
	$(ASTGEN_OR)/astgen_lib.$(OC_LIB_EXT) \
	../ast_config.$(OC_OBJ_EXT) \
	../meta_ast.$(OC_OBJ_EXT) \
	ast_accessors.ml \
	superast.ml \
	uplinks.ml \
	ast_graph.ml


TOCLEAN += ast_graph
ast_graph: $(AST_GRAPH_ML:.ml=.$(OC_OBJ_EXT))
	$(OCCC) $(OC_FLAGS) -o $@ $^


############################################################################
#### uplinks
#### 

TOCLEAN += uplinks_generated.ml
uplinks_generated.ml: \
		../gen_uplinks $(ASTGEN_OR)/ast.ast.oast
	../gen_uplinks $(ASTGEN_OR)/ast.ast.oast \
		-tr ../ast_reflection_control.txt \
		-o uplinks_generated

TOCLEAN += uplinks.ml
uplinks.ml: uplinks_header.ml uplinks_generated.ml \
		uplinks_trailer.ml
	echo "# 1 \"`pwd`/uplinks_header.ml\"" > $@
	cat uplinks_header.ml >> $@
	echo "# 1 \"`pwd`/uplinks_generated.ml\"" >> $@
	cat uplinks_generated.ml >> $@
	echo "# 1 \"`pwd`/uplinks_trailer.ml\"" >> $@
	cat uplinks_trailer.ml >> $@

uplinks.cmx: uplinks.cmi superast.cmx
uplinks.cmo: uplinks.cmi superast.cmi


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


superast.cmi: ast_accessors.$(OC_OBJ_EXT)
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
	$(OCC) $(OC_BYTE_FLAGS) $(OC_FLAGS) -I $(UTIL) -I $(GENERAL) \
		-I $(ASTGEN_OR) -I .. -c $<

TOCLEAN += *.cmx *.o
%.cmx: %.ml
	$(OCOPT) $(OC_FLAGS) -I $(UTIL) -I $(GENERAL) -I $(ASTGEN_OR) \
		-I .. -c $<

TOCLEAN += *.cmi
%.cmi: %.mli
	$(OCC) $(OC_FLAGS) -I $(UTIL) -I $(GENERAL) -I $(ASTGEN_OR) \
		-I .. -c $<


-include mldeps.mk

.PHONY: mldeps
depend: mldeps
mldeps:
	ocamldep $(OCDEPOPTS) -I $(UTIL) -I $(GENERAL) -I $(ASTGEN_OR) \
		*ml *mli > mldeps.mk
