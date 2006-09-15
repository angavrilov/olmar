
WEBHOST=solo
WEBDIR=web-docs/olmar
WEBSSH=$(WEBHOST):$(WEBDIR)
DATE=$(shell date +%Y.%m.%d)
OLMARTAR=elsa-2005.08.22b-olmar-$(DATE).tar.gz
all:

TODOCCLEAN =


.PHONY: copy-doc cp-index-html olmar-tar

DOCFILESCVS=\
	minor_gc.html \
	minor_gc-detail-small-border.png \
	minor_gc.dot

DOCFILESNOCVS=\
	hello-world.png \
	hello-world-border.png \
	crc.ii \
	crc.oast \
	crc.dot \
	crc.ps.gz \
	crc.png \
	crc-small.png \
	minor_gc-detail.png.gz \
	minor_gc.ps.gz \
	minor_gc-pages.ps.gz \
	minor_gc.fig.gz \
	minor_gc.png

DOCFILES= $(DOCFILESCVS) $(DOCFILESNOCVS)


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
	dot -v -Tps $< -o $@

TODOCCLEAN += minor_gc-pages.ps
minor_gc-pages.ps: minor_gc-pages.dot
	dot -v -Tps $< -o $@

TODOCCLEAN += minor_gc-pages.dot
minor_gc-pages.dot: minor_gc.dot
	head -1 $< > $@
	echo '    page="240,70";' >> $@
	tail -n +2 $< >> $@

TODOCCLEAN += minor_gc-size.dot
minor_gc-size.dot: minor_gc.dot
	head -1 $< > $@
	echo '    size="90,90";' >> $@
	tail -n +2 $< >> $@

TODOCCLEAN += minor_gc.fig
minor_gc.fig: minor_gc.dot
	dot -v -Tfig $< -o $@

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

crc.ps: crc.dot
	dot -Tps -o crc.ps crc.dot

TODOCCLEAN += crc.png
crc.png: crc.svg
	convert -resize 5000x crc.svg crc.png

TODOCCLEAN += crc-small.png
crc-small.png: crc.svg
	convert -resize 300x crc.svg crc-small.png


.PHONY: prepare-snapshot-doc
prepare-snapshot-doc: $(DOCFILES)

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


.PHONY: clean-doc
clean-doc:
	rm -f $(TODOCCLEAN)