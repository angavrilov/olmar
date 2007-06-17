readme.txt for Olmar 
--------------------

This release is provided under the BSD license.  See license.txt.

Elkhound is a parser generator.  
Elsa is a C/C++ parser that uses Elkhound.
Olmar is a patched Elsa version that can reflect the internal 
  C++ syntax tree into an Ocaml variant type.

The Olmar documentation is in asttools/doc/index.html. The
various other index.html files contain documentation for the
original Elkhound/Elsa system.

Alternatively, see http://www.cs.ru.nl/~tews/olmar/
and http://www.cs.berkeley.edu/~smcpeak/elkhound/ .


Build instructions:

  $ ./configure 
  $ make
  $ make check     (optional but a good idea)

This simply does each of these activities in each of the directories:
smbase, ast, elkhound and elsa.  If a command fails you can restart it
in a particular directory just by going into the failing directory and
issuing it there.

After building, the interesting binary is elsa/ccparse.  See
asttools/doc/index.html and elsa/index.html for more info on what
to do with it. 

Please email any problems/suggestions/patches to tews@cs.ru.nl.
