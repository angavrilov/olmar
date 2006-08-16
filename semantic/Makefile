
OCAMLC   := ocamlc.opt -g

#all: hashloc
all: test_ast
#all: path_to_node.cmo
#all: ast_graph
#all: ast_graph astiter.cmo astiternodes.cmo astmap.cmo dumpast hashloc

TOCLEAN =

TOCLEAN += nodes.dot nodes.ps
.PHONY: test_ast
test_ast: ast_graph
	./ast_graph ../elsa/in/t0566.cc.oast
	dot -Tps nodes.dot > nodes.ps

ASTGRAPH_ML=\
	dense_set.ml \
	../elsa/elsa_util.ml \
	../elsa/cc_ml_types.ml \
	../elsa/ml_ctype.ml \
	../elsa/ast_annotation.ml \
	ast_util.ml \
	ast_graph.ml

TOCLEAN += ast_graph
ast_graph: $(ASTGRAPH_ML:.ml=.cmo)
	$(OCAMLC) -o $@ $^

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

TOCLEAN += *.cmi
%.cmi: %.mli
	$(OCAMLC) -I ../elsa -c $<


-include mldeps.mk

.PHONY: mldeps
depend: mldeps
mldeps:
	ocamldep -I ../elsa *ml *mli > mldeps.mk
