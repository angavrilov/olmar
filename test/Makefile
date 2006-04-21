
ifeq ($(shell hostname), debian)
	GPP=g++-3.4
else
	GPP=g++
endif


all: testall

testall: testa testast testelsa

.PHONY: testast testelsa testa

testa:
	../ast/astgen a.ast
	ocamlc.opt -c a.ml
	g++ -Wall -c a.cc \
		-I ../ast -I ../smbase \
		-I /usr/local/stow/ocaml-3.09.1/lib/ocaml \
		-Wno-deprecated

FILES += ast/ast.ast
testast:
	../ast/astgen test-ast.ast

FILES += elsa/cc.ast elsa/cc_tcheck.ast elsa/cc_print.ast elsa/cc_elaborate.ast
FILES += elsa/gnu.ast elsa/kandr.ast
testelsa:
	../ast/astgen -otest-cc.ast.gen \
		test-cc.ast \
		test-cc_tcheck.ast \
		test-cc_print.ast \
		test-cfg.ast \
		test-cc_elaborate.ast \
		test-gnu.ast \
		test-kandr.ast

astgen.E: ../ast/astgen.cc
	$(GPP) -E -o astgen.E ../ast/astgen.cc -Wall \
		-D__UNIX__ -DNDEBUG -D__LINUX__ -I../smbase


.PHONY: astgen.xml

astgen.xml: astgen.xml.expr astgen.xml.type

astgen.xml.expr: astgen.E ../elsa/ccparse
	../elsa/ccparse -tr xmlPrintAST,xmlPrintAST-indent astgen.E \
		> astgen.xml.expr

astgen.xml.type: astgen.E ../elsa/ccparse
	../elsa/ccparse -tr xmlPrintAST,xmlPrintAST-indent,xmlPrintAST-types \
		astgen.E > astgen.xml.type


.PHONY: t.xml t.xml.type

t.E: t.c
	$(GPP) -E -o t.E t.c -Wall -D__UNIX__ -DNDEBUG -D__LINUX__ -I../smbase

t.xml: t.xml.expr t.xml.type

#		| sed -e 's/[A-Z]\{2,3\}0x[0-9a-f]*/ASTPOINTER/g' \
t.xml.expr: t.E 
	../elsa/ccparse \
		-tr xmlPrintAST,xmlPrintAST-indent \
		t.E \
		> t.xml.expr


#		| sed -e 's/[A-Z]\{2,3\}0x[0-9a-f]*/ASTPOINTER/g' \
t.xml.type: t.E 
	../elsa/ccparse \
		-tr xmlPrintAST,xmlPrintAST-indent,xmlPrintAST-types \
		t.E \
		> t.xml.type


.PHONY: diff

diff:
	for f in $(FILES); do \
	    echo ============================================================; \
	    echo =========== diff $$f test-`basename $$f` ===============; \
	    echo ============================================================; \
	    diff ../$$f test-`basename $$f`; \
	done



.PHONY: clean
clean:
	rm -f t.xml.* t.E
	rm -f astgen.{E,xml.type,xml.expr}
	rm -f a.{cc,h,xml}
	rm -f test-ast.{cc,h,xml,ml}
	rm -f test-cc.ast.gen.{cc,h,xml,ml}
	rm -f *.cmi *.cmo