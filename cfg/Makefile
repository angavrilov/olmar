#  Copyright 2006-2007, Hendrik Tews, All rights reserved.            #
#  See file license.txt for terms of use                              #
#######################################################################


.PHONY: all

all: cfg_lib.cmxa test
#all: cfg


# directories of other software; 
OLMARUTIL := ../olmar/util
GENERAL   := ../olmar/build/general
ELSA      := ../olmar/build/elsa
GENELSA   := ../olmar/meta/generated_elsa

# external tools
OCC   := ocamlc.opt
OCOPT := ocamlopt.opt
OCCC  := ocamlopt.opt

OC_OBJ_EXT := cmx
OC_LIB_EXT := cmxa
OC_FLAGS := -w Ae
OC_BYTE_FLAGS := -g

OCDEBUG=0
ifeq ($(OCDEBUG),1)
  OCOPT := $(OCC) -g
  OCCC  := $(OCC) -g
  OCC   := $(OCC) -g
  OC_OBJ_EXT := cmo
  OC_LIB_EXT := cma
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
	test/t008.cc\
	test/t009.cc\
	test/t010.cc\
	test/t012.cc

.PHONY: test
test: cfg $(TESTFILES:.cc=.oast) $(TESTFILES:.cc=.dot)
	for f in $(TESTFILES) ; do \
		./cfg -dot -o $$(dirname $$f)/$$(basename $$f .cc).cfg.dot \
			$$(dirname $$f)/$$(basename $$f .cc).oast; \
	done

CFG_ML=\
	cfg_type.ml \
	cfg_util.ml \
	build.ml \
	dot.ml

cfg_lib.cma: $(CFG_ML:.ml=.cmo)
	$(OCC) -a -o $@ $^

cfg_lib.cmxa: $(CFG_ML:.ml=.cmx)
	$(OCOPT) -a -o $@ $^

###########################################################################

CFG_PROG_ML=\
	$(OLMARUTIL)/olmar_utils.$(OC_LIB_EXT) \
	$(GENERAL)/astgen_general.$(OC_LIB_EXT) \
	$(ELSA)/elsa_lib.$(OC_LIB_EXT) \
	$(GENELSA)/elsa_ast_util.$(OC_LIB_EXT) \
	cfg_lib.$(OC_LIB_EXT) \
	main.ml

TOCLEAN += cfg
cfg: $(CFG_PROG_ML:.ml=.$(OC_OBJ_EXT))
	$(OCCC) $(OC_FLAGS) -o $@ $^


CFG_PROG_BYTE_ML=$(CFG_PROG_ML:.$(OC_LIB_EXT)=.cma)

TOCLEAN += cfg.byte
cfg.byte: $(CFG_PROG_BYTE_ML:.ml=.cmo)
	$(OCC) -g $(OC_FLAGS) -o $@ $^



.PHONY: clean
clean:
	rm -f $(TOCLEAN)
	rm -f test/*.o
	rm -f test/*.pml

OCINCLUDES := -I $(OLMARUTIL) -I $(GENERAL) -I $(ELSA) -I $(GENELSA)

TOCLEAN += *.cmo
%.cmo: %.ml
	$(OCC) $(OC_BYTE_FLAGS) $(OC_FLAGS) $(OCINCLUDES) -c $<

TOCLEAN += *.cmx *.o
%.cmx: %.ml
	$(OCOPT) $(OC_FLAGS) $(OCINCLUDES) -c $<

TOCLEAN += *.cmi
%.cmi: %.mli
	$(OCC) $(OC_FLAGS) $(OCINCLUDES) -c $<

TOCLEAN += test/*.oast
%.oast: %.cc
	$(ELSA)/ccparse_or -oc $@ $<

TOCLEAN += test/*.dot
%.dot: %.oast $(GENELSA)/ast_graph
	$(GENELSA)/ast_graph -o $@ $<


-include mldeps.mk

.PHONY: mldeps
depend: mldeps
mldeps:
	ocamldep $(OCINCLUDES) *ml *mli > mldeps.mk
