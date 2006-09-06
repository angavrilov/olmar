
WEBDIR=solo:web-docs/olmar


all:



.PHONY: copy-doc

DOCFILES=\
	index.html \
	minor_gc.html \
	hello-world.png \
	hello-world-border.png \
	crc.png \
	crc-small.png

DOCFILES2=\
	index.html \
	minor_gc-detail-small-border.png

copy-doc: $(DOCFILES2)
	scp $^ $(WEBDIR)
