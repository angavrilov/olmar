# $Id$
#
# Makefile for semantic

.PHONY: all
all: ast2pvs testall nova/buddy.pvs nova/ptab.pre.cpp nova/ptab.pvs

##########
# The semantics compiler itself (based on various elsa/asttools files, which
# we assume to be current).
##########

OC_OBJ_EXT := cmx
OC_LIB_EXT:=cmxa
OCC := ocamlc.opt
OCCC := ocamlopt.opt
OCOPT := ocamlopt.opt
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


AST2PVS_LIBS:=\
	../olmar/util/olmar_utils.$(OC_LIB_EXT) \
	../olmar/build/general/astgen_general.$(OC_LIB_EXT) \
	../olmar/build/elsa/elsa_lib.$(OC_LIB_EXT) \
	../olmar/meta/generated_elsa/elsa_ast_util.$(OC_LIB_EXT) \
	../cfg/cfg_lib.$(OC_LIB_EXT)



ast2pvs: option.ml ast2pvs.ml
	$(OCOPT) -w Ae -c option.ml
	$(OCOPT) -w Aexyz -I ../olmar/build/elsa -I ../olmar/build/general -I ../olmar/meta/generated_elsa -I ../olmar/util -I ../cfg -c ast2pvs.ml
	$(OCCC) -w Ae -o ast2pvs $(AST2PVS_LIBS) option.$(OC_OBJ_EXT) ast2pvs.$(OC_OBJ_EXT)


byte:
	$(MAKE) -C ../olmar byte
	$(MAKE) OCDEBUG=1 -C ../cfg all
	rm -f ast2pvs
	$(MAKE) OCDEBUG=1 ast2pvs



##########
# A number of test programs: C++ to PVS.
##########

# pre-processing of C++ sources
%.pre.cpp: %.cpp nova/include/*.h
	g++ -D NDEBUG -I nova/include/ -E -o $@ $<

# Ocaml abstract syntax trees
.PRECIOUS: %.oast
%.oast: %.pre.cpp
	../olmar/build/elsa/ccparse_or -oc $@ $<

# PVS theories
%.pvs: %.oast ast2pvs
	./ast2pvs $< $@

.PHONY: sync_master
sync_master: nova/ptab.oast ast2pvs
	./ast2pvs nova/ptab.oast nova/ptab-sync-master.pvs -function sync_master

.PHONY: htsync
htsync: sync_master
	cp nova/ptab-sync-master.pvs /home/tews/VFiasco/hardware

.PHONY: twsync
twsync: sync_master
	cp nova/ptab-sync-master.pvs ../../Robin/hardware

# PRECONDITION: 
# NOVA_DIR has been set to the absolute path where the Robin/nova repository has been checked out
# HARDWARE_DIR to the absolute path of Robin/hardware
nova_links: 
	-@ln -s $(NOVA_DIR)/ptab.cpp nova/ptab.cpp 2>/dev/zero
	-@ln -s $(NOVA_DIR)/include/*.h nova/include/ 2>/dev/zero

rm_nova_links:
	find -name "*.h" | grep -v "stdio.h" | xargs rm -f

links: nova_links sync_master
	-@ln -s $(PWD)/nova/ptab-sync-master.pvs $(HARDWARE_DIR)/ 2>/dev/zero

##########

# .dot graph files
%.dot: %.oast
	../olmar/meta/generated_elsa/ast_graph -real -o $@ $<

# PostScript files (from .dot)
%.ps: %.dot
	dot -Tps -o$@ $<

##########

# 'test/test???.cpp' -> 'test/test???.pvs'
.PHONY: testall
testall: $(patsubst %.cpp,%.pvs,$(wildcard test/test???.cpp))

##########
# clean
##########

.PHONY: clean
clean:
	rm -f *.cmi *.cmx *.o ast2pvs
	rm -f test/test???.pvs
	rm -f test/test???.oast
	rm -f nova/buddy.pvs nova/ptab.pre.cpp nova/ptab.pvs
	rm -f nova/ptab.oast
