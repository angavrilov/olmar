
Overview documentation for Elkhound parser generator
----------------------------------------------------


Overview
--------                                           
Elkhound takes as input a language grammar written in essentially BNF
(Backus-Naur Form) annotated with reduction actions, and outputs a C++
parser for that language.  The executable that does this is called
'gramanl' (grammar analyzer).

Additionally, the 'cc.gr' grammar is a grammar for C and C++.  By running
the parser generator on cc.gr, you get a parser for C/C++.  The Makefile
will generate the 'ccgr' executable which is this grammar's parser.


Subsystems
----------
The "../smbase" directory contains a general-purpose utility library.

The "../ast" directory contains a system for describing syntax trees
and getting implementations of many common tree-manipulation functions
automatically.  Partially inspired by ML's tree types.

This directory ("../parsgen") contains the parser generator code.



How to compile
--------------
  % ./configure
  % make
  % make check    (optional)

There is no 'make install' yet.  Compilation produces "gramanl", the
parser generator executable, and "libglr.a", the runtime library,


Module sets
-----------
Unless otherwise noted, all modules are a .cc file and an .h file.
Read the .h file first, it's shorter and has more "what this does"
type comments.  Some modules are in the ../ast/ subdirectory because
they are used there too; these are marked "(ast)".

The first set of modules, called 'grammar-set' in the Makefile, are
for representing grammars in memory.  They are:

  locstr:    (ast) Pair: source location and string table reference.

  grammar:   Terminal, Nonterminal, Production, Grammar, etc.  This
             is the core of the grammar representation.

  asockind:  Defines the AssocKind union.  Obscure.


The next set, 'grampar-set', is responsible for parsing grammar input
files and creating the Grammar and associated objects.  The modules are:

  asthelp:      (ast) Support file for 'gramast'.

  ccsstr:       (ast) Contains the knowledge needed to parse the embedded
                C++ code, e.g., finding the closing "}".

  embedded:     (ast) Interface to an embedded-language module.
                Generalization of 'ccsstr' for any language.

  fileloc:      (ast) Represents a location in a source file.  Useful for
                error reporting.

  gramlex:      (ast) Wrapper C++ class for the lexer.  Provides a cleaner
                interface than raw flex variable access.

  strtable:     (ast) Collection of immutable strings.

  emitcode:     Module for emitting code with #line directives.

  emittables:   Implementation of ParseTables::emitConstructionCode().
                This is separated from parsetables.cc so the runtime
                library can avoid linking with emitcode.cc.

  gramast:      AST for grammar files.

  gramlex.lex:  Flex scanner for the grammar input file.

  grampar.y:    Bison grammar for the grammar input file.  The actions
                in this file create an AST for the grammar.

  grampar:      Parses the AST produced by grampar.y and fills in a
                Grammar structure as it parses.

Next, 'glr-set' are the modules for the GLR parsing algorithm to use
at run time.  They are assembled into a library, libglr.a, which must
be linked with programs that use Elkhound parsers.

  fileloc:      (described above)

  cyctimer:     Processor cycle timer.  More convenient interface to
                smbase's 'cycles' module.

  glr:          The GLR algorithm itself.

  parsetables:  Representation, storage of LR parse tables.

  useract:      User-actions interface.  Contains an implementation
                of just NOPs.

Finally, 'gramanl-dep' are the modules that go into the parser
generator.

  gramanl:      The grammar analyzer; this is the parser generator.
  
  gramexpl:     A very incomplete interactive grammar explorer.  I
                stopped working on this as soon as I found the grammar
                bug I was looking for.  :)



                
The 'support-set' contains some modules that are used in most of my
testing and performance grammars.  It contains an aborted, currently
quite inefficient implementation of a C preprocessor (lexer2).

  asthelp:      (described above)
  strtable:     (described above)

  cc_lang:      C/C++ parsing options (e.g. which keywords to use).

  lexer1:       First-stage lexical analysis of C/C++ (see Phases
                of parsing, below).

  lexer1.lex:   Flex scanner for C/C++.

  lexer2:       Second-stage lexical analysis of C/C++.

  parssppt:     Some generic support routines for parsers.  Contains
                declarations for some of the emitted C++ code.


Other documentation
-------------------
There are a few other documentation files lying around:

  readme.txt    This file.

  types.txt     Describes the representation of C/C++ types.  Out of date.

  grammar.txt   Describes the format of grammar files like cc.gr.

  parsgen.txt   Some of the trace flags defined.  Currently incomplete.

  doc/          Additional documentation.  Currently, it has some diagrams
                which show some of the module dependencies (very helpful
                for navigating among these sources).


Directories
-----------

  ai            Test inputs for abstract interpretation; TODO: move.

  asfsdf        A few ASF+SDF grammar descriptions, mostly for performance
                comparison.

  c.in          C sample input.

  examples      Stand-alone examples of using Elkhound; designed to help
                the new user learn the tool.

  in            Input for various grammars, including some C++ input.

  out           Known-good outputs for some of the grammars and inputs;
                some of them might be out of date by now.

  triv          Various "trivial" grammars, mostly for performance testing
                or verifying that particular corner cases are handled
                correctly.


Individual files have a blurb at the top which describes their purpose,
so consult the files themselves to see what they are or do.


Phases of parsing for C/C++
---------------------------
The generated parser has several phases:
  Lexer1:    Partitions the input file(s) into tokens.
  Lexer2:    Applies interpretations (e.g. parses integers) to tokens.
  Parse:     Parses the tokens into a (possibly-ambiguous) tree.
  Semantics: Compute some semantics info, primarily to fully
             disambiguate the parse tree.

The lexers are slow.  They are part of an aborted attempt to design
a better preprocessing scheme for C.  They in fact do *no* preprocessing
now, so 'gcc -E' is required.


-- BEGIN out of date stuff --
Next, the 'cc-set' of modules is the implementation of C/C++
semantics.  The routines here are called by the semantic functions in
'cc.gr' to do the bulk of the language-specific stuff.

  cc_type:      Represents C/C++ types, such as "int" and "pointer to
                a function that returns a pointer to struct Foo".

  cc_env:       Environment; declarations result in mappings that get
                put into the environment.  Knows about scoping.

  cparse:       Simpler environment, capable only of making type/name
                distinction.

  gramanl:      Grammar analysis.  Given a grammar, it computes things
                like first/follow sets, LR item sets, etc.  This module
                also has the driver to emit the C++ semantic functions.
-- END out of date stuff --




