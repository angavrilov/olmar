# .gdbinit

file astgen
#file agrampar
break main
break breaker
#set args -tr tmp tiny.ast
set args ../parsgen/gramast.ast
run
