# $Id$
#
# Makefile for semantic

.PHONY: all
all: ast2pvs testall test.pvs test.ps buddy.pvs ptab.pvs

##########
# The semantics compiler itself (based on various elsa/asttools files, which
# we assume to be current).
##########

ast2pvs: option.ml ast2pvs.ml
	ocamlopt.opt -w Ae -c option.ml
	ocamlopt.opt -w Aexyz -I ../olmar/build/elsa -I ../olmar/build/general -I ../olmar/meta/generated_elsa -c ast2pvs.ml
	ocamlopt.opt -w Ae -o ast2pvs option.cmx ast2pvs.cmx

##########
# A number of test programs.
##########

# pre-processing of C++ sources
%.pre.cpp: %.cpp
	g++ -E -o $@ $<

# Ocaml abstract syntax trees
%.oast: %.pre.cpp
	../elsa/ccparse -oc $@ $<

# PVS theories
%.pvs: %.oast ast2pvs
	./ast2pvs $< $@

# .dot graph files
%.dot: %.oast
	../asttools/ast_graph -real -o $@ $<

# PostScript files (from .dot)
%.ps: %.dot
	dot -Tps -o$@ $<

# 'test???.cpp' -> 'test???.pvs'
.PHONY: testall
testall: $(patsubst %.cpp,%.pvs,$(wildcard test???.cpp))

##########
# clean
##########

.PHONY: clean
clean:
	rm -f *.cmi *.cmx *.o ast2pvs
	rm -f test*.pvs test.ps buddy.pvs ptab.pvs
