# .gdbinit

#set width 200

#  file ccgr
#  set args -tr cil-tree cc.bin tmp

#  break main
#  break breaker
#  run

#  file gramanl
#  set args -tr itemsets cexp3

#  file cexp2
#  set args cexp2.bin cexp.in1

#file grampar
#set args -tr cat-grammar cexp3.gr

file cexp3
set args -tr sval,parse,malloc cexp3.bin cexp3.in1

break main
break breaker
run
