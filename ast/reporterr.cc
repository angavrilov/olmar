// reporterr.cc            see license.txt for copyright and terms of use
// code for reporterr.h

#include "reporterr.h"      // this module

#include <iostream.h>       // cout


// --------------------- SimpleReportError -------------------------
void SimpleReportError::reportError(rostring str)
{
  cout << "error: " << str << endl;
}

void SimpleReportError::reportWarning(rostring str)
{
  cout << "warning: " << str << endl;
}

SimpleReportError simpleReportError;


// --------------------- SilentReportError -------------------------
void SilentReportError::reportError(rostring str)
{}

void SilentReportError::reportWarning(rostring str)
{}

SilentReportError silentReportError;


// EOF
