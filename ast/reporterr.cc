// reporterr.cc
// code for reporterr.h

#include "reporterr.h"      // this module

#include <iostream.h>       // cout

void SimpleReportError::reportError(char const *str)
{
  cout << "error: " << str << endl;
}

void SimpleReportError::reportWarning(char const *str)
{
  cout << "warning: " << str << endl;
}

SimpleReportError simpleReportError;
