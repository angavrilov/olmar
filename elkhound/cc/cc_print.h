// cc_print.h            see license.txt for copyright and terms of use
// declarations for C++ pretty-printer; the AST entry
// points are declared in cc.ast

#ifndef CC_PRINT_H
#define CC_PRINT_H

#include "cc.ast.gen.h"         // C++ AST; this module
#include "str.h"                // stringBuilder

#include <iostream.h>           // ostream

class code_output_stream {
  std::ostream *out;
  stringBuilder *sb;

  // true, write to 'sb'; false, write to 'out'
  bool using_sb;

public:
  code_output_stream(std::ostream &out)
    : out(&out), sb(NULL), using_sb(false) {}
  code_output_stream(stringBuilder &sb)
    : out(NULL), sb(&sb), using_sb(true) {}

  void flush() {
    if (!using_sb) out->flush();
  }

  #define MAKE_INSERTER(type)                          \
    code_output_stream & operator << (type message) {  \
      if (using_sb) *sb << message;                    \
      else *out << message;                            \
      flush();                                         \
      return *this;                                    \
    }

  MAKE_INSERTER(char const*)
  MAKE_INSERTER(bool)
  MAKE_INSERTER(int)
  MAKE_INSERTER(long)
  MAKE_INSERTER(double)

  #undef MAKE_INSERTER

  code_output_stream & operator << (ostream& (*manipfunc)(ostream& outs)) {
    if (using_sb) {
      // sm: just assume it's "endl"; the only better thing I could
      // imagine doing is pointer comparisons with some other
      // well-known omanips, since we certainly can't execute it...
      *sb << "\n";
    }
    else {
      *out << manipfunc;
      out->flush();
    }
    return *this;
  }
  
  // provide access to the built string
  stringBuilder const &getString() const
  {
    xassert(using_sb);
    return *sb;
  }
};

// allow a block to have the equivalent of a finally block; the
// "close" argument to the constructor is printed in the destructor.
class codeout {
  char *close;
  code_output_stream &out;
  public:
  codeout(code_output_stream &out,
          const char *message, char *open = "", char *close = "")
    : close(close), out(out)
  {
    out << const_cast<char *>(message);
    out << " ";
    out << open;
  }
  ~codeout() {
    out << close;
  }
};

class twalk_output_stream {
  std::ostream &out;
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
  twalk_output_stream(std::ostream &out, bool on = true)
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
  SourceLocation current_loc;
  
public:
  PrintEnv(ostream &out) : code_output_stream(out) {}
  PrintEnv(stringBuilder &sb) : code_output_stream(sb) {}
};


#endif // CC_PRINT_H
