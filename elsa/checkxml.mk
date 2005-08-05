#!/usr/bin/make checkxml

# DIFF := diff -u
DIFF := diff -c -b

PR := ./ccparse

.PHONY: all
all: clean check_ast check_type

TEST1 :=
TEST1 += t0001.cc
TEST1 += t0002.cc
TEST1 += t0003.cc
TEST1 += t0004.cc
TEST1 += t0005.cc
TEST1 += t0006.cc
TEST1 += t0007.cc
TEST1 += t0008.cc
TEST1 += t0009.cc
TEST1 += t0010.cc
TEST1 += t0011.cc
TEST1 += t0012.cc
TEST1 += t0013.cc
TEST1 += t0014.cc
TEST1 += t0014a.cc
TEST1 += t0015.cc
TEST1 += t0016.cc
TEST1 += t0017.cc
TEST1 += t0018.cc
TEST1 += t0019.cc
TEST1 += t0020.cc
TEST1 += t0021.cc
TEST1 += t0022.cc
TEST1 += t0023.cc
TEST1 += t0024.cc
TEST1 += t0025.cc
TEST1 += t0026.cc
TEST1 += t0027.cc
TEST1 += t0028.cc
TEST1 += t0029.cc
TEST1 += t0030.cc
TEST1 += t0030a.cc
TEST1 += t0030b.cc
TEST1 += t0031.cc
TEST1 += t0032.cc
TEST1 += t0033.cc
TEST1 += t0034.cc
TEST1 += t0035.cc
TEST1 += t0036.cc
TEST1 += t0037.cc
TEST1 += t0038.cc
TEST1 += t0039.cc
TEST1 += t0040.cc
TEST1 += t0041.cc
TEST1 += t0042.cc
TEST1 += t0043.cc
TEST1 += t0044.cc
TEST1 += t0045.cc
TEST1 += t0046.cc
TEST1 += t0047.cc
TEST1 += t0048.cc
TEST1 += t0049.cc
TEST1 += t0050.cc
TEST1 += t0051.cc
TEST1 += t0052.cc
TEST1 += t0053.cc
TEST1 += t0054.cc
TEST1 += t0055.cc
TEST1 += t0056.cc
TEST1 += t0057.cc
TEST1 += t0058.cc
TEST1 += t0059.cc
TEST1 += t0060.cc
TEST1 += t0061.cc
TEST1 += t0062.cc
TEST1 += t0063.cc
TEST1 += t0064.cc
TEST1 += t0065.cc
TEST1 += t0066.cc
TEST1 += t0067.cc
TEST1 += t0068.cc
TEST1 += t0069.cc
TEST1 += t0070.cc
TEST1 += t0071.cc
TEST1 += t0072.cc
TEST1 += t0073.cc
TEST1 += t0074.cc
TEST1 += t0075.cc
TEST1 += t0076.cc
TEST1 += t0077.cc
TEST1 += t0078.cc
TEST1 += t0079.cc
TEST1 += t0080.cc
TEST1 += t0081.cc
TEST1 += t0082.cc
TEST1 += t0083.cc
TEST1 += t0084.cc
TEST1 += t0085.cc
TEST1 += t0086.cc
TEST1 += t0087.cc
TEST1 += t0088.cc
TEST1 += t0089.cc
TEST1 += t0090.cc
TEST1 += t0091.cc
TEST1 += t0092.cc
TEST1 += t0093.cc
TEST1 += t0094.cc
TEST1 += t0095.cc
TEST1 += t0096.cc
TEST1 += t0097.cc
TEST1 += t0098.cc
TEST1 += t0099.cc
TEST1 += t0100.cc

TEST2 :=
TEST2 += t0001.cc
TEST2 += t0002.cc
TEST2 += t0003.cc
TEST2 += t0004.cc
TEST2 += t0005.cc
TEST2 += t0006.cc
TEST2 += t0007.cc
TEST2 += t0008.cc
TEST2 += t0009.cc
TEST2 += t0010.cc
TEST2 += t0011.cc
TEST2 += t0012.cc
TEST2 += t0013.cc
TEST2 += t0014.cc
TEST2 += t0014a.cc
TEST2 += t0015.cc
TEST2 += t0016.cc
TEST2 += t0017.cc
TEST2 += t0018.cc
TEST2 += t0019.cc
TEST2 += t0020.cc
TEST2 += t0021.cc
TEST2 += t0022.cc
TEST2 += t0023.cc
TEST2 += t0024.cc
TEST2 += t0025.cc

TOCLEAN :=

# check parsing commutes with xml serialization
T1D := $(addprefix outdir/,$(TEST1))
TOCLEAN += $(addsuffix .B0.cc,$(T1D)) $(addsuffix .B1.xml,$(T1D)) $(addsuffix .B2.xml.cc,$(T1D)) $(addsuffix .B3.diff,$(T1D))
$(addsuffix .B0.cc,$(T1D)): outdir/%.B0.cc: in/%
	$(PR) -tr no-elaborate,prettyPrint $< | ./chop_out > $@
$(addsuffix .B1.xml,$(T1D)): outdir/%.B1.xml: in/%
	$(PR) -tr no-elaborate,xmlPrintAST,xmlPrintAST-indent $< | ./chop_out > $@
$(addsuffix .B2.xml.cc,$(T1D)): outdir/%.B2.xml.cc: outdir/%.B1.xml
	$(PR) -tr no-elaborate,parseXml,prettyPrint $< | ./chop_out > $@
$(addsuffix .B3.diff,$(T1D)): outdir/%.B3.diff: outdir/%.B0.cc outdir/%.B2.xml.cc
	$(DIFF) $^ | tee $@
.PHONY: check_ast
check_ast: $(addsuffix .B3.diff,$(T1D))

# check typechecking commutes with xml serialization
T2D := $(addprefix outdir/,$(TEST2))
TOCLEAN += $(addsuffix .C0.cc,$(T2D)) $(addsuffix .C1.xml,$(T2D)) $(addsuffix .C2.xml.cc,$(T2D)) $(addsuffix .C3.diff,$(T2D))
$(addsuffix .C0.cc,$(T2D)): outdir/%.C0.cc: in/%
	$(PR) -tr no-elaborate,printTypedAST $< | ./filter_loc > $@
$(addsuffix .C1.xml,$(T2D)): outdir/%.C1.xml: in/%
	$(PR) -tr no-elaborate,xmlPrintAST,xmlPrintAST-indent,xmlPrintAST-types $< | ./chop_out > $@
$(addsuffix .C2.xml.cc,$(T2D)): outdir/%.C2.xml.cc: outdir/%.C1.xml
	$(PR) -tr no-typecheck,no-elaborate,parseXml,printAST $< | ./filter_loc > $@
$(addsuffix .C3.diff,$(T2D)): outdir/%.C3.diff: outdir/%.C0.cc outdir/%.C2.xml.cc
	$(DIFF) $^ | tee $@
.PHONY: check_type
check_type: $(addsuffix .C3.diff,$(T2D))

.PHONY: clean
clean:
	rm -f $(TOCLEAN)
