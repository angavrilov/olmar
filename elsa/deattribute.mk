# Makefile for building and testing deattribute

deattribute: deattribute.o
	$(CXX) -o $@ $^

.PRECIOUS: deattribute.cc
deattribute.cc: deattribute.lex
	flex -s -o$@ $<
	mv $@ lexer.tmp
	sed -e 's/class istream;/#include <iostream.h>/' <lexer.tmp >$@
	rm lexer.tmp

.PHONY: clean-deattribute
clean-deattribute:; rm -f deattribute.cc deattribute.o deattribute

DEATTR_IN :=
DEATTR_IN += t0001.cc
DEATTR_IN += t0002.cc
DEATTR_IN += t0003.cc
DEATTR_IN += t0004.cc
DEATTR_IN += t0005.cc
DEATTR_IN += t0006.cc
DEATTR_IN += t0007.cc
DEATTR_IN += t0008.cc
DEATTR_IN += t0009.cc
DEATTR_IN += t0010.cc
DEATTR_IN += t0011.cc
DEATTR_IN += t0012.cc
DEATTR_IN += t0013.cc
DEATTR_IN += t0014.cc
DEATTR_IN += t0014a.cc
DEATTR_IN += t0015.cc
DEATTR_IN += t0016.cc
DEATTR_IN += t0017.cc
DEATTR_IN += t0018.cc
DEATTR_IN += t0019.cc
DEATTR_IN += t0020.cc
DEATTR_IN += t0021.cc
DEATTR_IN += t0022.cc
DEATTR_IN += t0023.cc
DEATTR_IN += t0024.cc
DEATTR_IN += t0025.cc
DEATTR_IN += t0026.cc
DEATTR_IN += t0027.cc
DEATTR_IN += t0028.cc
DEATTR_IN += t0029.cc
DEATTR_IN += t0030.cc
DEATTR_IN += t0030a.cc
DEATTR_IN += t0030b.cc
DEATTR_IN += t0031.cc
DEATTR_IN += t0032.cc
DEATTR_IN += t0033.cc
DEATTR_IN += t0034.cc
DEATTR_IN += t0035.cc
DEATTR_IN += t0036.cc
DEATTR_IN += t0037.cc
DEATTR_IN += t0038.cc
DEATTR_IN += t0039.cc
DEATTR_IN += t0040.cc
DEATTR_IN += t0041.cc
DEATTR_IN += t0042.cc
DEATTR_IN += t0043.cc
DEATTR_IN += t0044.cc
DEATTR_IN += t0045.cc
DEATTR_IN += t0046.cc
DEATTR_IN += t0047.cc
DEATTR_IN += t0048.cc
DEATTR_IN += t0049.cc
DEATTR_IN += t0050.cc
DEATTR_IN += t0051.cc
DEATTR_IN += t0052.cc
DEATTR_IN += t0053.cc
DEATTR_IN += t0054.cc
DEATTR_IN += t0055.cc
DEATTR_IN += t0056.cc
DEATTR_IN += t0057.cc
DEATTR_IN += t0058.cc
DEATTR_IN += t0059.cc
DEATTR_IN += t0060.cc
DEATTR_IN += t0061.cc
DEATTR_IN += t0062.cc
DEATTR_IN += t0063.cc
DEATTR_IN += t0064.cc
DEATTR_IN += t0065.cc
DEATTR_IN += t0066.cc
DEATTR_IN += t0067.cc
DEATTR_IN += t0068.cc
DEATTR_IN += t0069.cc
DEATTR_IN += t0070.cc
DEATTR_IN += t0071.cc
DEATTR_IN += t0072.cc
DEATTR_IN += t0073.cc
DEATTR_IN += t0074.cc
DEATTR_IN += t0075.cc
DEATTR_IN += t0076.cc
DEATTR_IN += t0077.cc
DEATTR_IN += t0078.cc
DEATTR_IN += t0079.cc
DEATTR_IN += t0080.cc
DEATTR_IN += t0081.cc
DEATTR_IN += t0082.cc
DEATTR_IN += t0083.cc
DEATTR_IN += t0084.cc
DEATTR_IN += t0085.cc
DEATTR_IN += t0086.cc
DEATTR_IN += t0087.cc
DEATTR_IN += t0088.cc
DEATTR_IN += t0089.cc
DEATTR_IN += t0090.cc
DEATTR_IN += t0091.cc
DEATTR_IN += t0092.cc
DEATTR_IN += t0093.cc
DEATTR_IN += t0094.cc
DEATTR_IN += t0095.cc
DEATTR_IN += t0096.cc
DEATTR_IN += t0097.cc
DEATTR_IN += t0098.cc
DEATTR_IN += t0099.cc
DEATTR_IN += t0100.cc
DEATTR_IN += t0101.cc
DEATTR_IN += t0102.cc
DEATTR_IN += t0103.cc
DEATTR_IN += t0104.cc
DEATTR_IN += t0105.cc
DEATTR_IN += t0106.cc
DEATTR_IN += t0107.cc
DEATTR_IN += t0108.cc
DEATTR_IN += t0109.cc
DEATTR_IN += t0110.cc
DEATTR_IN += t0111.cc
DEATTR_IN += t0112.cc
DEATTR_IN += t0113.cc
DEATTR_IN += t0114.cc
DEATTR_IN += t0115.cc
DEATTR_IN += t0116.cc
DEATTR_IN += t0117.cc
DEATTR_IN += t0118.cc
DEATTR_IN += t0119.cc
DEATTR_IN += t0120.cc
DEATTR_IN += t0121.cc
DEATTR_IN += t0122.cc
DEATTR_IN += t0123.cc
DEATTR_IN += t0124.cc
DEATTR_IN += t0125.cc
# t0126.cc skipped since it actually contains attributes
DEATTR_IN += t0127.cc
DEATTR_IN += t0128.cc
DEATTR_IN += t0129.cc
DEATTR_IN += t0130.cc
DEATTR_IN += t0131.cc

.PHONY: check-deattribute
check-deattribute: $(addprefix check-deattribute-idem/in/,$(DEATTR_IN))
check-deattribute: check-deattribute/in/t0126.c

# should be idempotent on files not containing attributes
.PHONY: check-deattribute-idem/%
check-deattribute-idem/%:
	./deattribute < $* > $*.da_out
	diff -u $* $*.da_out
	rm $*.da_out

# should match the corresponding output file
.PHONY: check-deattribute/%
check-deattribute/%:
	./deattribute < $* > $*.da_out
	diff -u $*.da_cor $*.da_out
	rm $*.da_out
