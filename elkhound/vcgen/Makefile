# Makefile in vcgen

all: node.i node2.i owner1.i owner2.i fldinvar.i

owner1.i: owner1.h
owner2.i: owner1.h

%.i: %.c
	rm -f $*.i
	cpp -C $*.c > $*.i
	chmod a-w $*.i
