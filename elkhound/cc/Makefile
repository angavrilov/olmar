all:
	cd ..; $(MAKE)

CLEAN_PATTERNS := \
  *.o  \
  *~ \
  *.d \
  *.exe \
  *.gen.* \
  *.bin \
  *.tab.* \
  *.output \
  a.out \
  core

clean:
	rm -f $(CLEAN_PATTERNS)
	rm -f lexer1 lexer1yy.cc
	rm -f lexer2 
	rm -f cc.bin ccgr ccparse libccgr.a 
	rm -f bccgr
	rm -f bccgr.y cc.tok
	rm -f ccparse_twalk

# ****************
# Check idempotency of ccparse -tr prettyPrint on files in parsgen/cc.in.

TOCHECK :=
TOCHECK += cc.in1
TOCHECK += cc.in2
TOCHECK += cc.in3
TOCHECK += cc.in4
TOCHECK += cc.in5
TOCHECK += cc.in6
TOCHECK += cc.in7
TOCHECK += cc.in8
TOCHECK += cc.in9
TOCHECK += cc.in10
TOCHECK += cc.in11
TOCHECK += cc.in12
TOCHECK += cc.in13
TOCHECK += cc.in14
TOCHECK += cc.in14a

# DEFERRED: Won't print scope operator; When you print a PQname, you
# should print a qualifier string.  Ask the PQName for the qualifier
# string and print that.  Get the name out of the variable.
# TOCHECK += cc.in15

TOCHECK += cc.in16

# ABOVE: Won't print scope operator.
# TOCHECK += cc.in17

TOCHECK += cc.in18
TOCHECK += cc.in19
TOCHECK += cc.in20
TOCHECK += cc.in21
TOCHECK += cc.in22
TOCHECK += cc.in23
TOCHECK += cc.in24

# ABOVE: Scope operator not being printed.
# int A::func(B &b) ->
# int ( func)(class B & b) {
# TOCHECK += cc.in25

# DSW: Templates not being printed correctly when the template
# arguments are used within the template.
#TOCHECK += cc.in26
#TOCHECK += cc.in27
#TOCHECK += cc.in28

TOCHECK += cc.in29

# Anonymous classes and enums being printed incorrectly in different
# ways.  Wrote Scott if this is the parser or the printer.
#TOCHECK += cc.in30
# Smaller example of same problem.
#TOCHECK += cc.in30a

TOCHECK += cc.in31
TOCHECK += cc.in32
TOCHECK += cc.in33
TOCHECK += cc.in34

# template <class num>
# class TPoint {
# becomes:
# class TPoint {
# Plus has other template printing problems mentioned above.
#TOCHECK += cc.in35

# Same template printing problems.
#TOCHECK += cc.in36

TOCHECK += cc.in37
TOCHECK += cc.in38
TOCHECK += cc.in39
TOCHECK += cc.in40
TOCHECK += cc.in41
TOCHECK += cc.in42

# That's all for parsgen/cc.in.

.PHONY: idem/check
idem/check: idem/clean
	./check_idempotency $(addprefix ../cc.in/,$(TOCHECK))

.PHONY: idem/clean
idem/clean:
	rm -f $(addprefix ../cc.in/,cc.*.out_raw cc.*.out cc.*.out2_raw cc.*.out2 cc.*.diff)

# ****************
# Check idempotency of ccparse -tr prettyPrint on C and C++ files in
# cure_cpp/cc_examples.

TOCHECK2 :=

# ccparse doesn't seem to find and parse include files?  It doesn't
# seem to take a -I flag.
# TOCHECK2 += ./Box.cc

# TOCHECK2 += ./JustHello.c
# TOCHECK2 += ./JustWorld.c
TOCHECK2 += ./Test_005true.c
# TOCHECK2 += ./Test_010hello.c
TOCHECK2 += ./Test_105true.cc
# TOCHECK2 += ./Test_110hello.cc
# TOCHECK2 += ./Test_210box.cc
# TOCHECK2 += ./Test_220tbox.cc
# TOCHECK2 += ./Test_310hello.c
# TOCHECK2 += ./Test_315hello.c
# TOCHECK2 += ./Test_320hello_tbox.cc
# TOCHECK2 += ./Test_325hello_tbox.cc

TESTDIR2 := ../../cure_cpp/cc_examples/
idem/check2: idem/clean2
	./check_idempotency $(addprefix $(TESTDIR2),$(TOCHECK2))

.PHONY: idem/clean2
idem/clean2:
	rm -f $(addprefix $(TESTDIR2),$(foreach XXX,*.c *.cc,$(addprefix $(XXX),.out_raw .out .out2_raw .out2 .diff)))

# ****************

# Check in the things I work on.
DSW_STUFF := Makefile cc_twalk.cc cc.ast check_idempotency chop_out

.PHONY: commit
commit:
	ci -l -m"no message" $(DSW_STUFF)

dsw_changes.tar.gz:
	rm -f $@;
	tar cvzf $@ $(DSW_STUFF)
