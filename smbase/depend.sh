#!/bin/sh
# given some compiler command-line args (including source file),
# output a Makefile line of dependencies for compiling that source file

# stolen from http://www.tip.net.au/~millerp/rmch/recu-make-cons-harm.html

# invoke gcc's preprocessor to discover dependencies:
#   -MM   output Makefile rule, ignoring "#include <...>" lines
#         (so as to avoid dependencies on system headers)
#   -MG   treat missing headers as present and in cwd
# then invoke sed:
#   - remove any occurrances of system headers if they sneak in
#   - make the .d file itself depend on the same things the .o does
gcc -MM -MG "$@" |
  sed -e 's@ /[^ ]*@@g' -e 's@^\(.*\)\.o:@\1.d \1.o:@'
