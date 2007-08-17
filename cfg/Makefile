#  Copyright 2006-2007, Hendrik Tews, All rights reserved.            #
#  See file license.txt for terms of use                              #
#######################################################################


.PHONY: all

all: test
#all: cfg


# directories of other software; 
ELSA      := ../elsa
ASTTOOLS  := ../asttools

# external tools
OCAMLC   := ocamlc.opt
OCAMLOPT := ocamlopt.opt
OCAMLCC  := ocamlopt.opt

OCAML_OBJ_EXT := cmx
OCAML_LIB_EXT := cmxa
OCAML_FLAGS := -w Ae
OCAML_BYTE_FLAGS := -g

OCAMLDEBUG=0
ifeq ($(OCAMLDEBUG),1)
  OCAMLOPT := $(OCAMLC) -g
  OCAMLCC  := $(OCAMLC) -g
  OCAMLC   := $(OCAMLC) -g
  OCAML_OBJ_EXT := cmo
  OCAML_LIB_EXT := cma
endif

# list of files to clean in 'clean' (etc.) targets
# (these get added to below)
TOCLEAN =


TESTFILES=\
	test/t001.cc\
	test/t002.cc\
	test/t003.cc\
	test/t004.cc\
	test/t005.cc\
	test/t006.cc\
	test/t007.cc\
	test/t008.cc

.PHONY: test
test: cfg $(TESTFILES:.cc=.oast) $(TESTFILES:.cc=.dot)
	for f in $(TESTFILES) ; do \
		./cfg -dot -o $$(dirname $$f)/$$(basename $$f .cc).cfg.dot \
			$$(dirname $$f)/$$(basename $$f .cc).oast; \
	done

CFG_ML=\
	$(ELSA)/elsa_util.ml \
	$(ELSA)/cc_ml_types.ml \
	$(ELSA)/ast_annotation.ml \
	$(ASTTOOLS)/ast_accessors.ml \
	$(ELSA)/oast_header_version.ml \
	$(ELSA)/oast_header.ml \
	cfg_type.ml \
	cfg_util.ml \
	build.ml \
	dot.ml \
	main.ml

TOCLEAN += cfg
cfg: $(CFG_ML:.ml=.$(OCAML_OBJ_EXT))
	$(OCAMLCC) $(OCAML_FLAGS) -o $@ $^



.PHONY: clean
clean:
	rm -f $(TOCLEAN)
	rm -f test/*.o
	rm -f test/*.pml

TOCLEAN += *.cmo
%.cmo: %.ml
	$(OCAMLC) $(OCAML_BYTE_FLAGS) $(OCAML_FLAGS) -I $(ELSA) -I $(ASTTOOLS) -c $<

TOCLEAN += *.cmx *.o
%.cmx: %.ml
	$(OCAMLOPT) $(OCAML_FLAGS) -I $(ELSA) -I $(ASTTOOLS) -c $<

TOCLEAN += *.cmi
%.cmi: %.mli
	$(OCAMLC) $(OCAML_FLAGS) -I $(ELSA) -I $(ASTTOOLS) -c $<

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
	ocamldep -I $(ELSA) -I $(ASTTOOLS) *ml *mli > mldeps.mk
