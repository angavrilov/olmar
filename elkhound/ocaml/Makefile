# Makefile

# directories
ELKHOUND := ..

#tmp: lexer.cmo

all: tobjpool main

# using dependency strategy from 
# http://www.cs.berkeley.edu/~smcpeak/autodepend/autodepend.html
%.cmo: %.ml
	ocamlc -c -g $*.ml
	@ocamldep $*.ml > $*.d

# run elkhound on a grammar spec
# (for now I don't want to use the interface stuff)
# 
# cygwin bug...
#%.ml: %.gr $(ELKHOUND)/elkhound
%.ml: %.gr
	$(ELKHOUND)/elkhound -v $*.gr
	rm -f $*.mli

# run ocamllex on a lex spec
%.ml: %.mll
	ocamllex $*.mll

# NOTE: Module order is important!  OCaml module dependencies cannot
# be cyclic, and the order presented must respect the dependency order.

# test program for objpool module
TOBJPOOL_MODULES = \
  arraystack.cmo \
  objpool.cmo \
  tobjpool.cmo
tobjpool: $(TOBJPOOL_MODULES)
	ocamlc -o $@ -g $^

# these are the minimal set of modules needed for an elkhound parser
MINIMAL_MODULES = \
  smutil.cmo \
  useract.cmo \
  lexerint.cmo \
  parsetables.cmo \
  arraystack.cmo \
  objpool.cmo \
  glr.cmo

# these are modules for the test program; they include some specific 
# grammar tables/actions, a lexer, a deterministic LR parser, the
# parse-tree-printing stuff (ptreenode, ptreeact), and a main() function
MAIN_MODULES = \
  $(MINIMAL_MODULES) \
  arith.cmo \
  een.cmo \
  lrparse.cmo \
  ptreenode.cmo \
  ptreeact.cmo \
  lexer.cmo \
  main.cmo
main: $(MAIN_MODULES)
	ocamlc -o $@ -g $^

# dependencies
ALL_MODULES = \
  $(MAIN_MODULES) \
  tobjpool.cmo
-include $(ALL_MODULES:.cmo=.d)

# run it a bunch of ways
check:
	echo "2+3" | ./main arith lr
	echo "2+3" | ./main arith glr
	echo "2+3" | ./main een glr
	echo "2+3+4" | ./main een glr ptree

clean:
	rm -f *.cmo *.cmi arith.ml* een.ml* lexer.ml main tobjpool
