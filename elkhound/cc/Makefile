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

# This one segfaults at the point where it tries to access the field
# of an E_fieldAcc.  field is NULL.
# void E_fieldAcc::itwalk(Env &env)
# . . .
#     global_code_out << field->name;  <-------- here

# Not printing structs.
TOCHECK += cc.in9

#TOCHECK += cc.in10

# "extern" qualifiers are not printed
# TOCHECK += cc.in11

#TOCHECK += cc.in12

# segfault on field again.
#TOCHECK += cc.in13

# Also doesn't work.
#TOCHECK += cc.in14

# this is getting printed before constructors and destructors: <cdtor>
# TOCHECK += cc.in15

#TOCHECK += cc.in16

# constructor again
# TOCHECK += cc.in17

.PHONY: check_idem
check_idem: clean_idem
	./check_idempotency $(addprefix ../cc.in/,$(TOCHECK))

.PHONY: clean_idem
clean_idem:
	rm -f ../cc.in/cc.*.out ../cc.in/cc.*.out2

#	./check_idempotency $(patsubst ../cc.in/,,$(wildcard ../cc.in/cc.in*))

# Check in the things I work on.
DSW_STUFF := Makefile cc_twalk.cc cc.ast check_idempotency chop_out

.PHONY: commit
commit:
	ci -l -m"no message" $(DSW_STUFF)

dsw_changes.tar.gz:
	rm -f $@;
	tar cvzf $@ $(DSW_STUFF)
