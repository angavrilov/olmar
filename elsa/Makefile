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

# Using differen parameter var from prototype fails.
# TOCHECK += cc.in11

TOCHECK += cc.in12
TOCHECK += cc.in13

#  p = new int[x]; ->
# ( (p)= (new int []));
# TOCHECK += cc.in14

# this is getting printed before constructors and destructors: <cdtor>
# TOCHECK += cc.in15

TOCHECK += cc.in16

# constructor again
# TOCHECK += cc.in17

TOCHECK += cc.in18

# constructor again
# TOCHECK += cc.in19

# "converstion-operator" being printed for converstion operators.
# operator char ();
# char ( conversion-operator)();
# TOCHECK += cc.in20

TOCHECK += cc.in21

# Bitfields not being printed correctly:
# int x : 2; ->
# int x;
#TOCHECK += cc.in22

TOCHECK += cc.in23
TOCHECK += cc.in24

# Scope operator not being printed.
# int A::func(B &b) ->
# int ( func)(class B & b) {
# TOCHECK += cc.in25

# Templates not being printed.
#TOCHECK += cc.in26
#TOCHECK += cc.in27
#TOCHECK += cc.in28

TOCHECK += cc.in29

# default arguments not being printed
#   inline Foo(int *x); ->
# inline <cdtor> ( constructor-special)(int * x);
# TOCHECK += cc.in30

# ( (operator-delete) ( (x)));
# TOCHECK += cc.in31

# Anonymous unions printed incorrectly
# union (null) {
# TOCHECK += cc.in32

# <cdtor> thing again
#TOCHECK += cc.in33

# Stopping here as there are too many errors.

.PHONY: idem/check
idem/check: idem/clean
	./check_idempotency $(addprefix ../cc.in/,$(TOCHECK))

.PHONY: idem/clean
idem/clean:
	rm -f ../cc.in/cc.*.out_raw ../cc.in/cc.*.out ../cc.in/cc.*.out2_raw ../cc.in/cc.*.out2

#	./check_idempotency $(patsubst ../cc.in/,,$(wildcard ../cc.in/cc.in*))

# Check in the things I work on.
DSW_STUFF := Makefile cc_twalk.cc cc.ast check_idempotency chop_out

.PHONY: commit
commit:
	ci -l -m"no message" $(DSW_STUFF)

dsw_changes.tar.gz:
	rm -f $@;
	tar cvzf $@ $(DSW_STUFF)
