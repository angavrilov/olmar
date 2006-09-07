
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
	minor_gc-detail-small-border.png

copy-doc: olmar-tar $(DOCFILES) cp-index-html 
	scp -q $(DOCFILES) $(WEBSSH)

index.html.rel: index.html
	sed  "s/@RELEASEDATE@/$$(date +%Y.%m.%d)/" < $^ > $@

cp-index-html: index.html.rel
	scp -q $^ $(WEBSSH)/index.html
	rm $^

olmar-tar:
	if [ ! -f $(OLMARTAR) ] ; then echo make snapshot first; false; fi
	scp -q $(OLMARTAR) $(WEBSSH)

.PHONY: new-doc
new-doc: rm-doc copy-doc


rm-doc:
	ssh $(WEBHOST) rm -rf $(WEBDIR)
	ssh $(WEBHOST) mkdir $(WEBDIR)
