# Makefile in vcgen

all: node.i node2.i owner1.i owner2.i

%.i: %.c
	rm -f $*.i
	cpp -C $*.c > $*.i
	chmod a-w $*.i
