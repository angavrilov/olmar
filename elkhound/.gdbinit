# .gdbinit for running glr

#file glr
#set args -tr parse-tree -tr progress -tr parse c.gr bsort.c.in4

file lexer1
set args -tr lexer1

break main
break breaker
run
