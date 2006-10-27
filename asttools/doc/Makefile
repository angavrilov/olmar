
WEBHOST=solo
WEBDIR=web-docs/olmar
WEBSSH=$(WEBHOST):$(WEBDIR)
DATE=$(shell date +%Y.%m.%d)
OLMARTAR=elsa-2005.08.22b-olmar-$(DATE).tar.gz
ocamlsrc=/usr/local/src/ocaml-3.09.3

all: prepare-snapshot-doc

TODOCCLEAN =


.PHONY: copy-doc cp-index-html olmar-tar

DOCFILESCVS=\
	minor_gc.html

DOCFILESNOCVS=\
	hello-world.png \
	hello-world-border.png \
	crc.ii \
	crc.oast \
	crc.dot \
	crc.ps.gz \
	crc.png \
	crc-small.png \
	minor_gc.ps.gz \
	minor_gc.png \
	minor_gc.fig.gz \
	minor_gc-pages.ps.gz \
	minor_gc-detail.png.gz \
	minor_gc-detail-small-border.png

DOCFILES= $(DOCFILESCVS) $(DOCFILESNOCVS)


.PHONY: prepare-snapshot-doc
prepare-snapshot-doc: $(DOCFILES)


TODOCCLEAN += $(DOCFILESNOCVS)
copy-doc: $(DOCFILES) cp-index-html olmar-tar 
	scp -q $(DOCFILES) $(WEBSSH); true


TODOCCLEAN += index.html.rel
index.html.rel: index.html
	sed  "s/@RELEASEDATE@/$$(date +%Y.%m.%d)/" < $^ > $@


cp-index-html: index.html.rel
	scp -q $^ $(WEBSSH)/index.html; true
	rm $^

olmar-tar:
	if [ ! -f /tmp/tews/$(OLMARTAR) ] ; \
		then echo make snapshot first; false; fi
	scp -q /tmp/tews/$(OLMARTAR) $(WEBSSH); true

TODOCCLEAN += minor_gc.png
minor_gc.png: minor_gc.ps
	convert -resize 8000x $< $@

TODOCCLEAN += minor_gc.ps
minor_gc.ps: minor_gc-size.dot
	neato -n -s2 -v -Tps $< -o $@

TODOCCLEAN += minor_gc-pages.ps
minor_gc-pages.ps: minor_gc-pages.dot
	neato -n -s2 -v -Tps $< -o $@

TODOCCLEAN += minor_gc-pages.dot
minor_gc-pages.dot: minor_gc-layout.dot
	head -1 $< > $@
	echo '    page="240,70";' >> $@
	tail -n +2 $< >> $@

TODOCCLEAN += minor_gc-size.dot
minor_gc-size.dot: minor_gc-layout.dot
	head -1 $< > $@
	echo '    size="10,90";' >> $@
	tail -n +2 $< >> $@

TODOCCLEAN += minor_gc.fig
minor_gc.fig: minor_gc-layout.dot
	neato -n -s2 -v -Tfig $< -o $@

TODOCCLEAN += minor_gc-layout.dot
minor_gc-layout.dot: minor_gc.dot 
	dot -v minor_gc.dot -o minor_gc-layout.dot

TODOCCLEAN += minor_gc.dot
minor_gc.dot: minor_gc.oast ../ast_graph
	../ast_graph minor_gc.oast -o minor_gc.dot

TODOCCLEAN += minor_gc.oast
minor_gc.oast: ../../elsa/ccparse
	pwd=$$(pwd); \
	(cd $(ocamlsrc)/byterun; \
	gcc -DCAML_NAME_SPACE -O -fno-defer-pop -Wall -D_FILE_OFFSET_BITS=64 \
		-D_REENTRANT   -E -o minor_gc.i minor_gc.c; \
	$$pwd/../../elsa/ccparse -tr c_lang -oc $$pwd/minor_gc.oast minor_gc.i)



TODOCCLEAN += hello-world.oast
hello-world.oast: hello-world.c ../../elsa/ccparse
	../../elsa/ccparse -tr c_lang -oc hello-world.oast hello-world.c

TODOCCLEAN += hello-world.dot
hello-world.dot: hello-world.oast ../ast_graph
	../ast_graph hello-world.oast -o hello-world.dot

TODOCCLEAN += hello-world.png
hello-world.png: hello-world.dot
	dot -Tpng $< -o $@

TODOCCLEAN += hello-world-border.png
hello-world-border.png: hello-world.png
	convert -bordercolor blue -border 5 -bordercolor white -border 50 $< $@

TODOCCLEAN += crc.oast
crc.oast crc.ii: ../../smbase/crc.cpp
	$(MAKE) -C ../../smbase crc.oast
	cp ../../smbase/crc.{oast,ii} .

TODOCCLEAN += crc.dot
crc.dot: crc.oast ../ast_graph
	../ast_graph -o crc.dot crc.oast 

TODOCCLEAN += crc.svg
crc.svg: crc.dot
	dot -Tsvg -o crc.svg crc.dot

TODOCCLEAN += crc.ps
crc.ps: crc.dot
	dot -Tps -o crc.ps crc.dot

TODOCCLEAN += crc.png
crc.png: crc.svg
	convert -resize 10000x crc.svg crc.png

TODOCCLEAN += crc-small.png
crc-small.png: crc.svg
	convert -resize 700x crc.svg crc-small.png


.PHONY: copy-doc
copydoc: $(DOCFILES)
	cp $(DOCFILES) $(TARGETDIR)

TODOCCLEAN += *.gz
%.gz: %
	gzip -9 -c $^ > $@

.PHONY: new-doc
new-doc: rm-doc copy-doc


rm-doc:
	ssh $(WEBHOST) rm -rf $(WEBDIR)
	ssh $(WEBHOST) mkdir $(WEBDIR)


.PHONY: docclean
clean-doc: docclean
docclean:
	rm -f $(TODOCCLEAN)