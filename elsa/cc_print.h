// cc_print.h            see license.txt for copyright and terms of use
// declarations for C++ pretty-printer; the AST entry
// points are declared in cc.ast

#ifndef CC_PRINT_H
#define CC_PRINT_H

#include "cc_ast.h"             // C++ AST; this module
#include "str.h"                // stringBuilder

#include <iostream.h>           // ostream

string make_indentation(int n);
string indent_message(int n, rostring s);

class code_output_stream {
  ostream *out;
  stringBuilder *sb;

  // true, write to 'sb'; false, write to 'out'
  bool using_sb;

  // depth of indentation
  int depth;
  // number of buffered trailing newlines
  int buffered_newlines;

public:
  code_output_stream(ostream &out);
  code_output_stream(stringBuilder &sb);
  ~code_output_stream();

  void finish();
  void up();
  void down();
  void flush();

  #define MAKE_INSERTER(type)                          \
    code_output_stream & operator << (type message) {  \
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

  void raw_print_and_indent(string s);

  code_output_stream & operator << (char const *message);
  code_output_stream & operator << (ostream& (*manipfunc)(ostream& outs));
  code_output_stream & operator << (rostring message);
  // provide access to the built string
  stringBuilder const &getString() const;
};

// allow a block to have the equivalent of a finally block; the
// "close" argument to the constructor is printed in the destructor.
class codeout {
  char const *close;
  code_output_stream &out;
  public:
  codeout(code_output_stream &out, rostring message, rostring open, char const *close = "");
  codeout(code_output_stream &out, rostring message);
  ~codeout();
};

class twalk_output_stream {
  ostream &out;
//    FILE *out
  bool on;
  int depth;

  private:
  void indent() {
    out << endl;
//      fprintf(out, "\n");
    out.flush();
//      fflush(out);
    for(int i=0; i<depth; ++i) out << " ";
//      for(int i=0; i<depth; ++i) fprintf(out, " ");
    out.flush();
//      fflush(out);
    out << ":::::";
//      fprintf(out, ":::::");
    out.flush();
//      fflush(out);
  }

  public:
  twalk_output_stream(ostream &out, bool on = true)
//    twalk_output_stream(FILE *out, bool on = true)
    : out(out), on(on), depth(0) {}

  void flush() {out.flush();}
//    void flush() {fflush(out);}
  twalk_output_stream & operator << (char *message) {
    if (on) {
      indent();
      out << message;
//        fprintf(out, message);
      out.flush();
//        fflush(out);
    }
    return *this;
  }
  twalk_output_stream & operator << (ostream& (*manipfunc)(ostream& outs)) {
    if (on) out << manipfunc;
    return *this;
  }
  void down() {++depth;}
  void up() {--depth;}
};

extern twalk_output_stream twalk_layer_out;

class olayer {
  twalk_output_stream &out;
  public:
  olayer(char *message, twalk_output_stream &out = twalk_layer_out)
    : out(out) {
    out << message << endl;
    out.flush();
    out.down();
  }
  ~olayer() {out.up();}
};


// global context for a pretty-print
class PrintEnv : public code_output_stream {
public:
  SourceLoc current_loc;
  
public:
  PrintEnv(ostream &out) : code_output_stream(out) {}
  PrintEnv(stringBuilder &sb) : code_output_stream(sb) {}
};

// for printing types
//  class TypePrinter {
//  };

#define PRINT_AST(AST)               \
  do {                               \
    PrintEnv penv(cout);             \
    if (AST) AST->print(penv);       \
    else cout << "(PRINT_AST:null)"; \
    cout << endl;                    \
  } while(0)

#endif // CC_PRINT_H
