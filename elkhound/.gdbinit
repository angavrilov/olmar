# .gdbinit

file gramlex
#set args -tr progress,ambiguities cc.gr cc.in1

#file lexer1
#set args -tr lexer1

break main
break breaker
run < gramlex.in1

