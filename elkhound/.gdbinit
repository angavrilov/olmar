# .gdbinit

#set width 200

file ccgr
set args -tr cil-tree cc.bin tmp


break main
break breaker
run

