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

# SCOTT: Using different parameter var from prototype fails.
# sm: should be fixed
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

# DSW: Templates not being printed.
#TOCHECK += cc.in26
#TOCHECK += cc.in27
#TOCHECK += cc.in28

TOCHECK += cc.in29

# SCOTT: default arguments not being printed; unimplemented feature.
#   inline Foo(int *x); ->
# inline <cdtor> ( constructor-special)(int * x);
# TOCHECK += cc.in30

# SCOTT:
# ( (operator-delete) ( (x)));
# sm: should be fixed now
TOCHECK += cc.in31

TOCHECK += cc.in32
TOCHECK += cc.in33

# Stopping here as there are too many errors.

.PHONY: idem/check
idem/check: idem/clean
	./check_idempotency $(addprefix ../cc.in/,$(TOCHECK))

.PHONY: idem/clean
idem/clean:
	rm -f $(addprefix ../cc.in/,cc.*.out_raw cc.*.out cc.*.out2_raw cc.*.out2 cc.*.diff)

# Check in the things I work on.
DSW_STUFF := Makefile cc_twalk.cc cc.ast check_idempotency chop_out

.PHONY: commit
commit:
	ci -l -m"no message" $(DSW_STUFF)

dsw_changes.tar.gz:
	rm -f $@;
	tar cvzf $@ $(DSW_STUFF)
