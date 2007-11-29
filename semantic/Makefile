# $Id$
#
# Makefile for semantic

.PHONY: all
all: ast2pvs testall nova/buddy.pvs nova/ptab.pre.cpp nova/ptab.pvs

##########
# The semantics compiler itself (based on various elsa/asttools files, which
# we assume to be current).
##########

ast2pvs: option.ml ast2pvs.ml
	ocamlopt.opt -w Ae -c option.ml
	ocamlopt.opt -w Aexyz -I ../olmar/build/elsa -I ../olmar/build/general -I ../olmar/meta/generated_elsa -c ast2pvs.ml
	ocamlopt.opt -w Ae -o ast2pvs ../olmar/build/general/astgen_general.cmxa ../olmar/build/elsa/elsa_lib.cmxa ../olmar/meta/generated_elsa/elsa_ast_util.cmxa option.cmx ast2pvs.cmx

##########
# A number of test programs: C++ to PVS.
##########

# pre-processing of C++ sources
%.pre.cpp: %.cpp
	g++ -I include/ -E -o $@ $<

# Ocaml abstract syntax trees
%.oast: %.pre.cpp
	../olmar/build/elsa/ccparse_or -oc $@ $<

# PVS theories
%.pvs: %.oast ast2pvs
	./ast2pvs $< $@

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
	rm -f test/test*.pvs
	rm -f nova/buddy.pvs nova/ptab.pre.cpp nova/ptab.pvs
