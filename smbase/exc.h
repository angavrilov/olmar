// exc.h            see license.txt for copyright and terms of use
// exception classes for SafeTP project
// Scott McPeak, 1996-1998  This file is public domain.

// I apologize for the inconsistent naming in this module.  It is
// the product of an extended period of experimenting with naming
// conventions for exception-related concepts.  The names near the
// end of the file reflect my current preferences.

#ifndef EXC_H
#define EXC_H

#include "breaker.h"     // breaker
#include "typ.h"         // bool
#include "xassert.h"     // xassert, for convenience for #includers
#include "str.h"         // string
#include <iostream.h>    // ostream

// forward declarations
class stringBuilder;


// by using this macro, the debugger gets a shot before the stack is unwound
#ifdef THROW
#undef THROW
#endif
#define THROW(obj) \
  { breaker(); throw (obj); }


// this function returns true if we're in the process of unwinding the
// stack, and therefore destructors may want to avoid throwing new exceptions;
// for now, may return false positives, but won't return false negatives
bool unwinding();

// inside a catch expression, the unwinding() function needs a tweak; pass
// the caught expression, and this returns whether there any *additional*
// exceptions currently in flight
class xBase;
bool unwinding_other(xBase const &x);

// using unwinding() in destructors to avoid abort()
#define CAUTIOUS_RELAY           \
  catch (xBase &x) {             \
    if (!unwinding_other(x)) {   \
      throw;   /* re-throw */    \
    }                            \
  }


// -------------------- xBase ------------------
// intent is to derive all exception objects from this
class xBase {
protected:
  string msg;
    // the human-readable description of the exception

public:
  static bool logExceptions;
    // initially true; when true, we write a record of the thrown exception
    // to clog

  static int creationCount;
    // current # of xBases running about; used to support unrolling()

public:
  xBase(char const *m);    // create exception object with message 'm'
  xBase(xBase const &m);   // copy ctor
  virtual ~xBase();

  char const *why() const
    { return (char const*)msg; }

  void insert(ostream &os) const;
  friend ostream& operator << (ostream &os, xBase const &obj)
    { obj.insert(os); return os; }
    // print why
};

// equivalent to THROW(xBase(msg))
void xbase(char const *msg) NORETURN;


// -------------------- x_assert -----------------------
// thrown by _xassert_fail, declared in xassert.h
// throwing this corresponds to detecting a bug in the program
class x_assert : public xBase {
  string condition;          // text of the failed condition
  string filename;           // name of the source file
  int lineno;                // line number

public:
  x_assert(char const *cond, char const *fname, int line);
  x_assert(x_assert const &obj);
  ~x_assert();

  char const *cond() const { return (char const *)condition; }
  char const *fname() const { return (char const *)filename; }
  int line() const { return lineno; }
};


// ---------------------- xFormat -------------------
// throwing this means a formatting error has been detected
// in some input data; the program cannot process it, but it
// is not a bug in the program
class xFormat : public xBase {
  string condition;          // what is wrong with the input

public:
  xFormat(char const *cond);
  xFormat(xFormat const &obj);
  ~xFormat();

  char const *cond() const { return (char const*)condition; }
};

// compact way to throw an xFormat
void xformat(char const *condition) NORETURN;

// convenient combination of condition and human-readable message
#define checkFormat(cond, message) \
  ((cond)? (void)0 : xformat(message))

// assert-like interface to xFormat
void formatAssert_fail(char const *cond, char const *file, int line) NORETURN;

#define formatAssert(cond) \
  ((cond)? (void)0 : formatAssert_fail(#cond, __FILE__, __LINE__))

  
// -------------------- XOpen ---------------------
// thrown when we fail to open a file
class XOpen : public xBase {
public:
  string filename;
  
public:
  XOpen(char const *fname);
  XOpen(XOpen const &obj);
  ~XOpen();
};

void throw_XOpen(char const *fname) NORETURN;


// -------------------- XOpenEx ---------------------
// more informative
class XOpenEx : public XOpen {
public:
  string mode;         // fopen-style mode string, e.g. "r"
  string cause;        // errno-derived failure cause, e.g. "no such file"

public:
  XOpenEx(char const *fname, char const *mode, char const *cause);
  XOpenEx(XOpenEx const &obj);
  ~XOpenEx();
                                              
  // convert a mode string as into human-readable participle,
  // e.g. "r" becomes "reading"
  static string interpretMode(char const *mode);
};

void throw_XOpenEx(char const *fname, char const *mode, char const *cause) NORETURN;


// ------------------- XUnimp ---------------------
// thrown in response to a condition that is in principle
// allowed but not yet handled by the existing code
class XUnimp : public xBase {
public:
  XUnimp(char const *msg);
  XUnimp(XUnimp const &obj);
  ~XUnimp();
};

void throw_XUnimp(char const *msg) NORETURN;

// throw XUnimp with file/line info
void throw_XUnimp(char const *msg, char const *file, int line) NORETURN;

#define xunimp(msg) throw_XUnimp(msg, __FILE__, __LINE__)


// ------------------- XFatal ---------------------
// thrown in response to a user action that leads to an unrecoverable
// error; it is not due to a bug in the program
class XFatal : public xBase {
public:
  XFatal(char const *msg);
  XFatal(XFatal const &obj);
  ~XFatal();
};

void throw_XFatal(char const *msg) NORETURN;


#endif // EXC_H

