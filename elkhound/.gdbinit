# .gdbinit for running glr

file glr
set args -tr progress,ambiguities c.gr c.in3

#file lexer1
#set args -tr lexer1

break main
break breaker
run
