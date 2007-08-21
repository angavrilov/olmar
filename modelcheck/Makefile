#  Copyright 2006-2007, Hendrik Tews, All rights reserved.            #
#  See file license.txt for terms of use                              #
#######################################################################


.PHONY: all

all: test
#all: kernel_prop_model


# directories of other software; 
ELSA      := ../elsa
ASTTOOLS  := ../asttools
CFG	  := ../cfg

# external tools
OCAMLC   := ocamlc.opt
OCAMLOPT := ocamlopt.opt
OCAMLCC  := ocamlopt.opt

OCAML_OBJ_EXT := cmx
OCAML_LIB_EXT := cmxa
OCAML_FLAGS := -w Ae
OCAML_BYTE_FLAGS := -g


# list of files to clean in 'clean' (etc.) targets
# (these get added to below)
TOCLEAN =


.PHONY: test
test: kernel_prop_model test/t001.oast test/t001.dot
	./kernel_prop_model -fn f test/t001.oast


KERNEL_PROP_MODEL_ML=\
	$(ELSA)/elsa_util.ml \
	$(ELSA)/ast_annotation.ml \
	$(ELSA)/oast_header_version.ml \
	$(ELSA)/oast_header.ml \
	$(ELSA)/cc_ml_types.ml \
	$(ASTTOOLS)/ast_accessors.ml \
	$(ASTTOOLS)/ast_util.ml \
	$(ASTTOOLS)/dense_set.ml \
	$(ASTTOOLS)/superast.ml \
	$(CFG)/cfg_type.ml \
	$(CFG)/cfg_util.ml \
	$(CFG)/build.ml \
	kernel_prop_model.ml

TOCLEAN += kernel_prop_model
kernel_prop_model: $(KERNEL_PROP_MODEL_ML:.ml=.$(OCAML_OBJ_EXT))
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ unix.$(OCAML_LIB_EXT) $^



.PHONY: clean
clean:
	rm -f $(TOCLEAN)
	rm -f test/*.o
	rm -f test/*.pml

TOCLEAN += *.cmo
%.cmo: %.ml
	$(OCAMLC) $(OCAML_BYTE_FLAGS) $(OCAML_FLAGS) \
		-I $(CFG) -I $(ELSA) -I $(ASTTOOLS) -c $<

TOCLEAN += *.cmx *.o
%.cmx: %.ml
	$(OCAMLOPT) $(OCAML_FLAGS) -I $(CFG) -I $(ELSA) -I $(ASTTOOLS) -c $<

TOCLEAN += *.cmi
%.cmi: %.mli
	$(OCAMLC) $(OCAML_FLAGS) -I $(CFG) -I $(ELSA) -I $(ASTTOOLS) -c $<

TOCLEAN += test/*.oast
%.oast: %.cc
	$(ELSA)/ccparse -oc $@ $<

TOCLEAN += test/*.dot
%.dot: %.oast
	$(ASTTOOLS)/ast_graph -o $@ $<


-include mldeps.mk

.PHONY: mldeps
depend: mldeps
mldeps:
	ocamldep -I $(CFG) -I $(ELSA) -I $(ASTTOOLS) *ml *mli > mldeps.mk
