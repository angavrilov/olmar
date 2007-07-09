
WEBHOST=lilo
WEBDIR=web-docs/olmar
WEBSSH=$(WEBHOST):$(WEBDIR)
DATE=$(shell date +%Y.%m.%d)
OLMARTAR=elsa-olmar-xx.tar.gz
ocamlsrc=/usr/local/src/ocaml-3.09.3

all: prepare-snapshot-doc

TODOCCLEAN =


.PHONY: cp-index-html olmar-tar

DOCFILESCVS=\
	minor_gc.html

MINOR_GC_TARGETS=\
	minor_gc.ps.gz \
	minor_gc.png \
	minor_gc.fig.gz \
	minor_gc-pages.ps.gz \
	minor_gc-detail.png.gz \
	minor_gc-detail-small-border.png

DOCFILESNOCVS=\
	hello-world.png \
	hello-world-border.png \
	crc.ii \
	crc.oast \
	crc.dot \
	crc.ps.gz \
	crc.png \
	crc-small.png \
	$(MINOR_GC_TARGETS)

DOCFILES= $(DOCFILESCVS) $(DOCFILESNOCVS)


.PHONY: prepare-snapshot-doc
prepare-snapshot-doc: $(DOCFILES)


TODOCCLEAN += $(DOCFILESNOCVS)


#SNAPSHOTNAME=elsa-9-491-282-1045-2476-olmar-2007-05-21
SNAPSHOTNAME=elsa-x-x-x-x-x-olmar-yyyy-mm-dd

TODOCCLEAN += index.html.rel
.PHONY: index.html.rel
index.html.rel: index.html
	sed  "s/@RELEASEDATE@/$(SNAPSHOTNAME)/" < $^ > $@

.PHONY: webdir
webdir:
	ssh $(WEBHOST) mkdir -p $(WEBDIR)

cp-index-html: webdir index.html.rel
	scp -q index.html.rel $(WEBSSH)/index.html; true
	scp -q old-releases.html $(WEBSSH)/releases/index.html; true

olmar-tar: webdir
	if [ ! -f /tmp/tews/$(OLMARTAR) ] ; \
		then echo make snapshot first; false; fi
	scp -q /tmp/tews/$(OLMARTAR) $(WEBSSH)/releases; true

.PHONY: copydoc-tar copydoc-www 
copydoc-tar: $(DOCFILES)
	cp $(DOCFILES) $(TARGETDIR)
	$(MAKE) index.html.rel
	cp index.html.rel $(TARGETDIR)/index.html

copydoc-www: $(DOCFILES) webdir cp-index-html olmar-tar 
	scp -q $(DOCFILES) $(WEBSSH); true

.PHONY: new-doc
new-doc: rm-doc copydoc-www

rm-doc:
	ssh $(WEBHOST) rm -f $(WEBDIR)/*; true

# convinience target for creating all minor_gc stuff over night
.PHONY: all-minor-gc
all-minor-gc: 
	echo -n "start "
	date
	$(MAKE) -k $(MINOR_GC_TARGETS); echo -n "end date "; date

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
	echo '    page="220,90";' >> $@
	tail -n +2 $< >> $@

ifeq ($(TOUCH_GC_DETAIL),1)
minor_gc-detail.png: minor_gc-pages.ps
	touch $@
else
TODOCCLEAN += minor_gc-detail.png
minor_gc-detail.png: minor_gc-pages.ps
	echo cut some interesting part and save it as minor_gc-detail.png
	false
endif

TODOCCLEAN += minor_gc-detail-small.png
minor_gc-detail-small.png: minor_gc-detail.png
	convert -resize 2000x $< $@

TODOCCLEAN += minor_gc-detail-small-border.png
minor_gc-detail-small-border.png: minor_gc-detail-small.png
	convert -bordercolor blue -border 5 -bordercolor white -border 50 $< $@

TODOCCLEAN += minor_gc-size.dot
minor_gc-size.dot: minor_gc.oast ../ast_graph
	../ast_graph -size minor_gc.oast -o minor_gc-size.dot

TODOCCLEAN += minor_gc.fig
minor_gc.fig: minor_gc.dot
	dot -v -Tfig $< -o $@

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
	convert -bordercolor blue -border 20 -bordercolor white -border 50 $< $@

TODOCCLEAN += crc.oast
crc.oast crc.ii: ../../smbase/crc.cpp
	$(MAKE) -C ../../smbase crc.oast
	cp ../../smbase/crc.{oast,ii} .

TODOCCLEAN += crc.dot
crc.dot: crc.oast ../ast_graph
	../ast_graph -o crc.dot crc.oast 

TODOCCLEAN += crc-size.dot
crc-size.dot: crc.oast ../ast_graph
	../ast_graph -size -o crc-size.dot crc.oast 

TODOCCLEAN += crc.svg
crc.svg: crc.dot
	dot -Tsvg -o crc.svg crc.dot

TODOCCLEAN += crc.ps
crc.ps: crc-size.dot
	dot -Tps -o crc.ps crc-size.dot

TODOCCLEAN += crc.png
crc.png: crc.svg
	convert -resize 10000x $< $@

TODOCCLEAN += crc-small.png
crc-small.png: crc.svg
	convert -resize 500x $< $@


.PHONY: touchdoc
touchdoc: 
	touch crc.dot crc.ps crc.svg crc.png crc-small.png crc-size.dot \
		hello-world.oast hello-world.dot \
		minor_gc.oast \
		minor_gc.dot minor_gc-size.dot minor_gc-pages.dot \
		minor_gc-pages.ps minor_gc.ps minor_gc.fig \
		minor_gc-detail.png minor_gc-detail-small.png $(DOCFILES)

TODOCCLEAN += *.gz
%.gz: %
	gzip -9 -c $^ > $@


.PHONY: docclean
clean-doc: docclean
docclean:
	rm -f $(TODOCCLEAN)