// cc_print.h            see license.txt for copyright and terms of use
// declarations for C++ pretty-printer; the AST entry
// points are declared in cc.ast

#ifndef CC_PRINT_H
#define CC_PRINT_H

#include "cc.ast.gen.h"         // C++ AST

class code_output_stream {
  std::ostream &out;

  public:
  code_output_stream(std::ostream &out) : out(out) {}
  void flush() {out.flush();}

  code_output_stream & operator << (const char *message) {
    out << message;
    out.flush();
    return *this;
  }
  code_output_stream & operator << (bool message) {
    out << message;
    out.flush();
    return *this;
  }
  code_output_stream & operator << (int message) {
    out << message;
    out.flush();
    return *this;
  }
  code_output_stream & operator << (long message) {
    out << message;
    out.flush();
    return *this;
  }
  code_output_stream & operator << (double message) {
    out << message;
    out.flush();
    return *this;
  }
  code_output_stream & operator << (ostream& (*manipfunc)(ostream& outs)) {
    out << manipfunc;
    out.flush();
    return *this;
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
};


#endif // CC_PRINT_H
