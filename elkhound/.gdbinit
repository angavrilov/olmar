# .gdbinit for running glr

file glr
set args -tr grammar -tr debug expr.gr expr.in1
break main
break breaker
run
