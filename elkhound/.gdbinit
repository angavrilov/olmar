# .gdbinit   -*- sh -*-

# needed for Squish
#set width 0

#file ccgr
#set args cc.bin c.in1

#set args -tr predicates cc.bin vcgen/owner1.i

#set args -tr factflow cc.bin vcgen/find2.c

#set args -tr stopAfterTCheck cc.bin c.in3

#set args cc.bin c.in1

#set args -tr factflow cc.bin vcgen/ff.c

#  break main
#  break breaker
#  run

#file gramanl
#set args -tr conflict,lrtable ite

file ite.gr.exe
set args -tr parse ite.in3

#file aSEb.gr.exe
#set args aSEb.in1


#file cexp3b
#set args -tr refct,sval cexp3b.bin cexp3.in1

#file ccgr
#set args -tr trivialActions,stopAfterParse cc.bin c.in4b

#file cexp3b
#set args cexp3b.bin cexp3.in1

#file grampar
#set args -tr cat-grammar cexp3.gr

#file cexp3
#set args -tr parse cexp3.bin cexp3.in1

#file cdecl2
#set args -tr cdecl cdecl2.bin cdecl.in1

break main
break breaker
run
