# Makefile

tmp: tobjpool

all: tobjpool main

%.cmo: %.ml
	ocamlc -c -g $*.ml
	@ocamldep $*.ml > $*.d

# dependencies
ALL_MODULES = \
  arraystack.cmo \
  objpool.cmo \
  tobjpool.cmo \
  main.cmo
-include $(ALL_MODULES:.cmo=.d)

tobjpool: arraystack.cmo objpool.cmo tobjpool.cmo
	ocamlc -o $@ -g $^

main: main.cmo
	ocamlc -o $@ -g $^

clean:
	rm -f *.cmo *.cmi main
