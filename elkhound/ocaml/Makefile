# Makefile

# directories
ELKHOUND := ..

#tmp: glr.cmo

all: tobjpool main

# using dependency strategy from 
# http://www.cs.berkeley.edu/~smcpeak/autodepend/autodepend.html
%.cmo: %.ml
	ocamlc -c -g $*.ml
	@ocamldep $*.ml > $*.d

# run elkhound on a grammar spec
# (for now I don't want to use the interface stuff)
%.ml: %.gr $(ELKHOUND)/elkhound
	$(ELKHOUND)/elkhound -v $*.gr
	rm -f $*.mli

# NOTE: Module order is important!  OCaml module dependencies cannot
# be cyclic, and the order presented must respect the dependency order.

TOBJPOOL_MODULES = \
  arraystack.cmo \
  objpool.cmo \
  tobjpool.cmo
tobjpool: $(TOBJPOOL_MODULES)
	ocamlc -o $@ -g $^

MAIN_MODULES = \
  useract.cmo \
  lexerint.cmo \
  parsetables.cmo \
  lrparse.cmo \
  arraystack.cmo \
  objpool.cmo \
  glr.cmo \
  arith.cmo \
  een.cmo \
  main.cmo
main: $(MAIN_MODULES)
	ocamlc -o $@ -g $^

# dependencies
ALL_MODULES = \
  $(MAIN_MODULES) \
  tobjpool.cmo
-include $(ALL_MODULES:.cmo=.d)

clean:
	rm -f *.cmo *.cmi arith.ml* main tobjpool
