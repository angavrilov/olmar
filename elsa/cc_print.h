// cc_print.h            see license.txt for copyright and terms of use
// declarations for C++ pretty-printer; the AST entry
// points are declared in cc.ast

#ifndef CC_PRINT_H
#define CC_PRINT_H

#include "cc_ast.h"             // C++ AST; this module
#include "str.h"                // stringBuilder

#include <iostream.h>           // ostream

// indents the source code sent to it
class CodeOutputStream {
  ostream *out;
  stringBuilder *sb;

  // true, write to 'sb'; false, write to 'out'
  bool using_sb;

  // depth of indentation
  int depth;
  // number of buffered trailing newlines
  int buffered_newlines;

  public:
  CodeOutputStream(ostream &out);
  CodeOutputStream(stringBuilder &sb);
  ~CodeOutputStream();

  void finish();
  void up();
  void down();
  void flush();

  #define MAKE_INSERTER(type)                          \
    CodeOutputStream & operator << (type message) {  \
      if (using_sb) *sb << message;                    \
      else *out << message;                            \
      flush();                                         \
      return *this;                                    \
    }

  MAKE_INSERTER(bool)
  MAKE_INSERTER(int)
  MAKE_INSERTER(unsigned int)
  MAKE_INSERTER(long)
  MAKE_INSERTER(unsigned long)
  MAKE_INSERTER(double)

  #undef MAKE_INSERTER

  static string makeIndentation(int n);
  static string indentMessage(int n, rostring s);

  void rawPrintAndIndent(string s);

  CodeOutputStream & operator << (char const *message);
  CodeOutputStream & operator << (ostream& (*manipfunc)(ostream& outs));
  CodeOutputStream & operator << (rostring message);
  // provide access to the built string
  stringBuilder const &getString() const;
};

// print paired delimiters, the second one is delayed until the end of
// the stack frame; that is, it is printed in the destructor.
class PairDelim {
  char const *close;
  CodeOutputStream &out;
  public:
  PairDelim(CodeOutputStream &out, rostring message, rostring open, char const *close = "");
  PairDelim(CodeOutputStream &out, rostring message);
  ~PairDelim();
};

// an output stream for printing comments that will indent them
// according to the level of the tree walk
class TreeWalkOutputStream {
  ostream &out;
  bool on;
  int depth;

  public:
  TreeWalkOutputStream(ostream &out, bool on = true);

  private:
  void indent();

  public:
  void flush();
  TreeWalkOutputStream & operator << (char *message);
  TreeWalkOutputStream & operator << (ostream& (*manipfunc)(ostream& outs));
  void down();
  void up();
};

extern TreeWalkOutputStream treeWalkOut;

// a class to make on the stack at ever frame of the tree walk that
// will automatically manage the indentation level of the
// TreeWalkOutputStream given
class TreeWalkDebug {
  TreeWalkOutputStream &out;
  public:
  TreeWalkDebug(char *message, TreeWalkOutputStream &out = treeWalkOut);
  ~TreeWalkDebug();
};

// This class knows how to print out Types to a stringBuilder or as a
// string; Underneath, the type is printed to a string builder; this
// design is necessary for situations where the order in which the
// tree is visited is not the same as the order in which they must be
// output; FIX: it would be best if instead of being forced to make a
// string and return it, if there were a superclass of OStream and
// stringBuilder so it could just print the output to it directly so
// that we avoid a layer of buffering when the printing and visiting
// order are the same.  That is an optimization I will leave for
// later.
class TypePrinter {
  public:
  // this method does the work
  virtual void print(Type *, stringBuilder &);
  // convenience method
  virtual string print(Type *);
};

// global context for a pretty-print
struct PrintEnv {
  CodeOutputStream &out;
  TypePrinter &typePrinter;
  SourceLoc loc;
  
  public:
  PrintEnv(CodeOutputStream &out0, TypePrinter &typePrinter0)
    : out(out0)
    , typePrinter(typePrinter0)
    , loc(SL_UNKNOWN)
  {}
};

#define PRINT_AST(AST)               \
  do {                               \
    PrintEnv penv(cout);             \
    if (AST) AST->print(penv);       \
    else cout << "(PRINT_AST:null)"; \
    cout << endl;                    \
  } while(0)

#endif // CC_PRINT_H
