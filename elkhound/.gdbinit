# .gdbinit for running glr

file glr
set args -tr progress,ambiguities,parse,conditions cc.gr tmp

#file lexer1
#set args -tr lexer1

break main
break breaker
run
