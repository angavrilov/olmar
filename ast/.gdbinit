# .gdbinit   -*- sh -*-

file ccsstr
#file agrampar
break main
break breaker
set args example.ast
set print static-members off
#set args ast.ast
#set args ../parsgen/gramast.ast
run
