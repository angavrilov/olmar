// reporterr.cc            see license.txt for copyright and terms of use
// code for reporterr.h

#include "reporterr.h"      // this module

#include <iostream.h>       // cout


// --------------------- SimpleReportError -------------------------
void SimpleReportError::reportError(char const *str)
{
  cout << "error: " << str << endl;
}

void SimpleReportError::reportWarning(char const *str)
{
  cout << "warning: " << str << endl;
}

SimpleReportError simpleReportError;


// --------------------- SilentReportError -------------------------
void SilentReportError::reportError(char const *str)
{}

void SilentReportError::reportWarning(char const *str)
{}

SilentReportError silentReportError;


// EOF
