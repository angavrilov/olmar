# .gdbinit   -*- sh -*-

#set width 200

file ccgr
#set args -tr printPaths,printTypedAST,stopAfterTCheck,tcheck cc.bin tcheck/loops.c

#set args -tr predicates cc.bin vcgen/global.c

set args -tr stopAfterTCheck cc.bin tcheck/loops.c

#  break main
#  break breaker
#  run

#file gramanl
#set args cc

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
