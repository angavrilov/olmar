#  Copyright 2006-2007, Hendrik Tews, All rights reserved.            #
#  See file license.txt for terms of use                              #
#######################################################################

#   13  wine ./probcc2
#   14  ls
#   15  cd liquor/LiQDist/LIQBIN/
#   16  ls
#   17  wine ./probcc2
#   18  po
#   19  pp ~/src/elsa-trunk/modelcheck/test/
#   20  wine ~/src/liquor/LiQDist/LIQBIN/probcc2.exe t001.probmela 
#   21  pp
#   22  wine ./liquor.exe 


.PHONY: all

all: test
#all: kernel_prop_model


# directories of other software; 
OLMARUTIL := ../olmar/util
GENERAL   := ../olmar/build/general
ELSA      := ../olmar/build/elsa
GENELSA   := ../olmar/meta/generated_elsa
CFG	  := ../cfg

# external tools
OCC   := ocamlc.opt
OCOPT := ocamlopt.opt
OCCC  := ocamlopt.opt

OC_OBJ_EXT := cmx
OC_LIB_EXT := cmxa
OC_FLAGS := -w Ae
OC_BYTE_FLAGS := -g

OC_INC := -I $(GENERAL) -I $(ELSA) -I $(CFG)


# list of files to clean in 'clean' (etc.) targets
# (these get added to below)
TOCLEAN =


.PHONY: test
test: kernel_prop_model test/t001.oast test/t001.dot
	./kernel_prop_model -fn f test/t001.oast -o test/t001.probmela


KERNEL_PROP_MODEL_ML=\
	$(OLMARUTIL)/olmar_utils.$(OC_LIB_EXT) \
	$(GENERAL)/astgen_general.$(OC_LIB_EXT) \
	$(ELSA)/elsa_lib.$(OC_LIB_EXT) \
	$(GENELSA)/elsa_ast_util.$(OC_LIB_EXT) \
	$(CFG)/cfg_lib.$(OC_LIB_EXT) \
	kernel_prop_model.ml

TOCLEAN += kernel_prop_model
kernel_prop_model: $(KERNEL_PROP_MODEL_ML:.ml=.$(OC_OBJ_EXT))
	$(OCCC) $(OC_FLAGS) -o $@ unix.$(OC_LIB_EXT) $^



.PHONY: clean
clean:
	rm -f $(TOCLEAN)
	rm -f test/*.o
	rm -f test/*.pml

TOCLEAN += *.cmo
%.cmo: %.ml
	$(OCC) $(OC_BYTE_FLAGS) $(OC_FLAGS) $(OC_INC) -c $<

TOCLEAN += *.cmx *.o
%.cmx: %.ml
	$(OCOPT) $(OC_FLAGS) $(OC_INC) -c $<

TOCLEAN += *.cmi
%.cmi: %.mli
	$(OCC) $(OC_FLAGS) $(OC_INC) -c $<

TOCLEAN += test/*.oast
%.oast: %.cc
	$(ELSA)/ccparse_or -oc $@ $<

TOCLEAN += test/*.dot
%.dot: %.oast
	$(GENELSA)/ast_graph -o $@ $<


-include mldeps.mk

.PHONY: mldeps
depend: mldeps
mldeps:
	ocamldep $(OC_INC) *ml *mli > mldeps.mk
