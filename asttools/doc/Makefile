
WEBHOST=solo
WEBDIR=web-docs/olmar
WEBSSH=$(WEBHOST):$(WEBDIR)
DATE=$(shell date +%Y.%m.%d)
OLMARTAR=elsa-2005.08.22b-olmar-$(DATE).tar.gz
all:



.PHONY: copy-doc cp-index-html olmar-tar

DOCFILES=\
	minor_gc.html \
	hello-world.png \
	hello-world-border.png \
	crc.png \
	crc-small.png \
	minor_gc-detail-small-border.png \
	minor_gc-detail.png.gz \
	minor_gc.ps.gz \
	minor_gc-pages.ps.gz \
	minor_gc.fig.gz \
	minor_gc.dot \
	minor_gc.png

copy-doc: $(DOCFILES) cp-index-html olmar-tar 
	scp -q $(DOCFILES) $(WEBSSH)

index.html.rel: index.html
	sed  "s/@RELEASEDATE@/$$(date +%Y.%m.%d)/" < $^ > $@

cp-index-html: index.html.rel
	scp -q $^ $(WEBSSH)/index.html
	rm $^

olmar-tar:
	if [ ! -f /tmp/tews/$(OLMARTAR) ] ; \
		then echo make snapshot first; false; fi
	scp -q /tmp/tews/$(OLMARTAR) $(WEBSSH)

minor_gc.ps: minor_gc-size.dot
	dot -v -Tps $< -o $@

minor_gc-pages.ps: minor_gc-pages.dot
	dot -v -Tps $< -o $@

minor_gc-pages.dot: minor_gc.dot
	head -1 $< > $@
	echo '    page="240,70";' >> $@
	tail -n +2 $< >> $@

minor_gc-size.dot: minor_gc.dot
	head -1 $< > $@
	echo '    size="90,90";' >> $@
	tail -n +2 $< >> $@

minor_gc.fig: minor_gc.dot
	dot -v -Tfig $< -o $@

.PHONY: prepare-snapshot-doc
prepare-snapshot-doc: $(DOCFILES)

.PHONY: copy-doc
copydoc: $(DOCFILES)
	cp $(DOCFILES) $(TARGETDIR)

%.gz: %
	gzip -9 -c $^ > $@

.PHONY: new-doc
new-doc: rm-doc copy-doc


rm-doc:
	ssh $(WEBHOST) rm -rf $(WEBDIR)
	ssh $(WEBHOST) mkdir $(WEBDIR)
