# Makefile

all: main

%.cmo: %.ml
	ocamlc -c -g $*.ml

main: main.cmo
	ocamlc -o $@ -g $^

clean:
	rm -f *.cmo *.cmi main
