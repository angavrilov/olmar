# Makefile for olmar/util
#
#  Copyright 2006-2007 Hendrik Tews, All rights reserved. #
#  See file license.txt for terms of use                  #
###########################################################

# Makefile.in
# this is the Makefile for the ocaml ast tools

# use the first goal for development, for release put it in comment and use all

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
stage_1: olmar_utils.$(OCAML_LIB_EXT)

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


#############################################################################
#####
##### gen_uplinks

OLMAR_UTILS_ML=\
	more_string.ml \
	dense_set.ml \
	dot_graph.ml

TOCLEAN += olmar_utils.cmxa
olmar_utils.cmxa: $(OLMAR_UTILS_ML:.ml=.cmx)
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ -a $^

TOCLEAN += olmar_utils.cma
olmar_utils.cma: $(OLMAR_UTILS_ML:.ml=.cmo)
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ -a $^


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
	$(OCAMLC) $(OCAML_BYTE_FLAGS) $(OCAML_FLAGS) -c $<

TOCLEAN += *.cmx *.o
%.cmx: %.ml
	$(OCAMLOPT) $(OCAML_FLAGS) -c $<

TOCLEAN += *.cmi
%.cmi: %.mli
	$(OCAMLC) $(OCAML_FLAGS) -c $<


-include mldeps.mk

.PHONY: mldeps
depend: mldeps
mldeps:
	ocamldep $(OCAMLDEPOPTS) *ml *mli > mldeps.mk
