# .gdbinit

#set width 200

#  file ccgr
#  set args -tr cil-tree cc.bin tmp

#  break main
#  break breaker
#  run

#  file gramanl
#  set args -tr semant cc

file cexp2
set args cexp2.bin cexp.in1

break main
break breaker
run
