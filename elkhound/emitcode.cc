// emitcode.cc
// code for emitcode.h

#include "emitcode.h"      // this module
#include "syserr.h"        // xsyserror

EmitCode::EmitCode(char const *f)
  : stringBuilder(),
    os(f),
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
  char const *p = pcharc();
  while (*p) {
    if (*p == '\n') {
      line++;
    }
    p++;
  }

  os << *this;
  setlength(0);
}
