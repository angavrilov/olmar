# .gdbinit   -*- sh -*-

file astgen
#file agrampar
break main
break breaker
set args example.ast
#set args ast.ast
#set args ../parsgen/gramast.ast
run
