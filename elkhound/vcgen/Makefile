# Makefile in vcgen

all: node.i node2.i

%.i: %.c
	rm -f $*.i
	cpp $*.c > $*.i
	chmod a-w $*.i
