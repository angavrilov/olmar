# $Id$
#
# Makefile for semantic

.PHONY: all
all: ast2pvs testall test.ps buddy.pvs

##########
# The semantics compiler itself (based on various elsa/asttools files, which
# we assume to be current).
##########

ast2pvs: option.ml ast2pvs.ml
	ocamlopt.opt -w Ae -I ../asttools/ -I ../elsa -c option.ml
	ocamlopt.opt -w Aexyz -I ../asttools/ -I ../elsa -I . -c ast2pvs.ml
	ocamlopt.opt -w Ae -o ast2pvs ../elsa/elsa_util.cmx ../elsa/cc_ml_types.cmx ../elsa/ast_annotation.cmx ../elsa/oast_header_version.cmx ../elsa/oast_header.cmx ../asttools/ast_accessors.cmx ../asttools/dense_set.cmx ../asttools/superast.cmx ../asttools/uplinks.cmx option.cmx ast2pvs.cmx

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

.PHONY: testall
testall: test001.pvs test002.pvs test003.pvs test.pvs

##########
# clean
##########

.PHONY: clean
clean:
	rm -f *.cmi *.cmx *.o ast2pvs
	rm -f test*.pvs test.ps buddy.pvs
