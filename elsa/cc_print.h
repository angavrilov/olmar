// cc_print.h            see license.txt for copyright and terms of use
// declarations for C++ pretty-printer; the AST entry
// points are declared in cc.ast

#ifndef CC_PRINT_H
#define CC_PRINT_H

#include "cc_ast.h"             // C++ AST; this module
#include "str.h"                // stringBuilder

#include <iostream.h>           // ostream

string make_indentation(int n);
string indent_message(int n, string s);

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
  code_output_stream(ostream &out)
    : out(&out), sb(NULL), using_sb(false), depth(0), buffered_newlines(0) {}
  code_output_stream(stringBuilder &sb)
    : out(NULL), sb(&sb), using_sb(true), depth(0), buffered_newlines(0) {
  }
  ~code_output_stream() {
    if (buffered_newlines) {
      cout << "**************** ERROR.  "
           << "You called my destructor before making sure all the buffered newlines\n"
           << "were flushed (by, say, calling finish())\n";
    }
  }

  void finish() {
    // NOTE: it is probably an error if depth is ever > 0 at this point.
//      printf("BUFFERED NEWLINES: %d\n", buffered_newlines);
    stringBuilder s;
    for(;buffered_newlines>1;buffered_newlines--) s << "\n";
    raw_print_and_indent(indent_message(depth,s));
    xassert(buffered_newlines == 1 || buffered_newlines == 0);
    if (buffered_newlines) {
      buffered_newlines--;
      raw_print_and_indent(string("\n")); // don't indent after last one
    }
  }

  void up() {
//      printf("UP %d\n", depth);
    depth--;
  }
  void down() {
    depth++;
//      printf("DOWN %d\n", depth);
  }
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

  MAKE_INSERTER(bool)
  MAKE_INSERTER(int)
  MAKE_INSERTER(unsigned int)
  MAKE_INSERTER(long)
  MAKE_INSERTER(unsigned long)
  MAKE_INSERTER(double)

  #undef MAKE_INSERTER

  void raw_print_and_indent(string s) {
    if (using_sb) *sb << s;
    else *out << s;
    flush();
  }

  code_output_stream & operator << (char const *message) {
    int len = strlen(message);
    if (len<1) return *this;
    char *message1 = strdup(message);

    int pending_buffered_newlines = 0;
    if (message1[len-1] == '\n') {
      message1[len-1] = '\0';    // whack it
      pending_buffered_newlines++;
    }

    stringBuilder message2;
    if (buffered_newlines) {
      message2 << "\n";
      buffered_newlines--;
    }
    message2 << message1;
    buffered_newlines += pending_buffered_newlines;

    raw_print_and_indent(indent_message(depth, message2));
    return *this;
  }

  code_output_stream & operator << (ostream& (*manipfunc)(ostream& outs)) {
    if (using_sb) {
      // sm: just assume it's "endl"; the only better thing I could
      // imagine doing is pointer comparisons with some other
      // well-known omanips, since we certainly can't execute it...
      if (buffered_newlines) {
        *sb << "\n";
        *sb << make_indentation(depth);
      } else buffered_newlines++;
    }
    else {
      // dsw: just assume its endl
//        *out << manipfunc;
      if (buffered_newlines) {
        *out << endl;
        *out << make_indentation(depth);
      } else buffered_newlines++;
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
    out << message;
    out << " ";
    out << open;
    if (strchr(open, '{')) out.down();
  }
  ~codeout() {
    if (strchr(close, '}')) out.up();
    out << close;
  }
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

#define PRINT_AST(AST)               \
  do {                               \
    PrintEnv penv(cout);             \
    if (AST) AST->print(penv);       \
    else cout << "(PRINT_AST:null)"; \
    cout << endl;                    \
  } while(0)

#endif // CC_PRINT_H
