// reporterr.h            see license.txt for copyright and terms of use
// interface for reporting errors and warnings

#ifndef REPORTERR_H
#define REPORTERR_H

#include "str.h"        // rostring

class ReportError {
public:                                     
  // report an error; 'str' should not have a newline
  virtual void reportError(rostring str)=0;

  // report a warning
  virtual void reportWarning(rostring str)=0;
};


// print messages to stdout with "error: " or "warning: " prepended
class SimpleReportError : public ReportError {
public:
  virtual void reportError(rostring str);
  virtual void reportWarning(rostring str);
};

extern SimpleReportError simpleReportError;


// throw away messages
class SilentReportError : public ReportError {
public:
  virtual void reportError(rostring str);
  virtual void reportWarning(rostring str);
};

extern SilentReportError silentReportError;



#endif // REPORTERR_H
