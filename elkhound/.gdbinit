# .gdbinit   -*- sh -*-

# needed for Squish
#set width 0

#file ccgr
#set args cc.bin c.in1

#set args -tr predicates cc.bin vcgen/struct.c

#set args -tr factflow cc.bin vcgen/find2.c

#set args -tr stopAfterTCheck cc.bin vcgen/if.c

#set args cc.bin c.in1

#set args -tr factflow cc.bin vcgen/ff.c

#  break main
#  break breaker
#  run

file gramanl
set args -tr conflict cc

#  file cexp2
#  set args cexp2.bin cexp.in1

#file grampar
#set args -tr cat-grammar cexp3.gr

#file cexp3
#set args -tr parse cexp3.bin cexp3.in1

#file cdecl2
#set args -tr cdecl cdecl2.bin cdecl.in1

break main
break breaker
run
