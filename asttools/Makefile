
OCAMLC   := ocamlc.opt -g
OCAMLOPT := ocamlopt.opt 

#all: hashloc
all: test_ast
#all: path_to_node.cmo
#all: ast_graph
#all: ast_graph astiter.cmo astiternodes.cmo astmap.cmo dumpast hashloc

TOCLEAN =

TOCLEAN += nodes.dot nodes.ps
.PHONY: test_ast
# -skip 840
test_ast: ast_graph
	./ast_graph ../elsa/in/gnu/g0010.cc.oast
	dot -Tps nodes.dot > nodes.ps

ASTGRAPH_ML=\
	dense_set.ml \
	../elsa/elsa_util.ml \
	../elsa/cc_ml_types.ml \
	../elsa/ml_ctype.ml \
	../elsa/ast_annotation.ml \
	ast_util.ml \
	ast_graph.ml

TOCLEAN += ast_graph.byte
ast_graph.byte: $(ASTGRAPH_ML:.ml=.cmo)
	$(OCAMLC) -o $@ $^

TOCLEAN += ast_graph.native
ast_graph.native: $(ASTGRAPH_ML:.ml=.cmx)
	$(OCAMLOPT) -o $@ $^

TOCLEAN += ast_graph
ast_graph: ast_graph.native
	cp ast_graph.native ast_graph

TOCLEAN += dumpast
dumpast: dumpast.ml
	$(OCAMLC) -o $@ -I ../elsa unix.cma $^

HASHLOC_ML=\
	dense_set.ml \
	../elsa/ast_annotation.ml \
	ast_util.ml \
	hashloc.ml

TOCLEAN += hashloc
hashloc: $(HASHLOC_ML:.ml=.cmo)
	$(OCAMLC) -o $@ $^


../elsa/%.cmo: ../elsa/%.ml
	$(MAKE) -C ../elsa $@

.PHONY: clean
clean:
	rm -f $(TOCLEAN)

TOCLEAN += *.cmo
%.cmo: %.ml
	$(OCAMLC) -I ../elsa -c $<

%.cmx: %.ml
	$(OCAMLOPT) -I ../elsa -c $<

TOCLEAN += *.cmi
%.cmi: %.mli
	$(OCAMLC) -I ../elsa -c $<


-include mldeps.mk

.PHONY: mldeps
depend: mldeps
mldeps:
	ocamldep -I ../elsa *ml *mli > mldeps.mk
