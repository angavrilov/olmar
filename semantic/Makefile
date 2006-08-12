
OCAMLC   := ocamlc.opt

all: test_ast
#all: ast_graph

.PHONY: test_ast
test_ast: ast_graph
	./ast_graph ../elsa/in/t0009.cc.oast
	dot -Tps nodes.dot > nodes.ps

ASTGRAPH_ML=\
	dense_set.ml \
	../elsa/elsa_util.ml \
	../elsa/cc_ml_types.ml \
	../elsa/ml_ctype.ml \
	../elsa/ast_annotation.ml \
	ast_util.ml \
	ast_graph.ml

ast_graph: $(ASTGRAPH_ML:.ml=.cmo)
	$(OCAMLC) -o $@ $^

../elsa/%.cmo: ../elsa/%.ml
	$(MAKE) -C ../elsa $@

%.cmo: %.ml
	$(OCAMLC) -I ../elsa -c $<

%.cmi: %.mli
	$(OCAMLC) -I ../elsa -c $<


-include mldeps.mk

.PHONY: mldeps
depend: mldeps
mldeps:
	ocamldep -I ../elsa *ml *mli > mldeps.mk
