// emitcode.cc            see license.txt for copyright and terms of use
// code for emitcode.h

#include "emitcode.h"      // this module
#include "syserr.h"        // xsyserror
#include "srcloc.h"        // SourceLoc
#include "trace.h"         // tracingSys

EmitCode::EmitCode(rostring f)
  : stringBuilder(),
    os(f.c_str()),
    fname(f),
    line(1)
{
  if (!os) {
    xsyserror("open", fname);
  }
}

EmitCode::~EmitCode()
{
  flush();
}


int EmitCode::getLine()
{
  flush();
  return line;
}


void EmitCode::flush()
{
  // count newlines
  char const *p = c_str();
  while (*p) {
    if (*p == '\n') {
      line++;
    }
    p++;
  }

  os << *this;
  setlength(0);
}


char const *hashLine()
{                   
  if (tracingSys("nolines")) {
    // emit with comment to disable its effect
    return "// #line ";
  }
  else {
    return "#line "; 
  }
}


// note that #line must be preceeded by a newline
string lineDirective(SourceLoc loc)
{
  char const *fname;
  int line, col;
  sourceLocManager->decodeLineCol(loc, fname, line, col);

  return stringc << hashLine() << line << " \"" << fname << "\"\n";
}

stringBuilder &restoreLine(stringBuilder &sb)
{
  // little hack..
  EmitCode &os = (EmitCode&)sb;

  // +1 because we specify what line will be *next*
  int line = os.getLine()+1;
  return os << hashLine() << line
            << " \"" << os.getFname() << "\"\n";
}
