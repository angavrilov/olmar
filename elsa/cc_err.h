// cc_err.h            see license.txt for copyright and terms of use
// objects for representing errors in C++ code

#ifndef CC_ERR_H
#define CC_ERR_H

#include "macros.h"    // ENUM_BITWISE_OR
#include "str.h"       // string
#include "srcloc.h"    // SourceLoc


// flags on errors
enum ErrorFlags {
  EF_NONE          = 0x00,

  // informative, but not an error; typically, such messages should
  // have a switch to disable them
  EF_WARNING       = 0x01,

  // when this is true, the error message should be considered
  // when disambiguation; when it's false, it's not a sufficiently
  // severe error to warrant discarding an ambiguous alternative;
  // for the most part, only environment lookup failures are
  // considered to disambiguate
  EF_DISAMBIGUATES = 0x02,

  EF_ALL           = 0x03
};
ENUM_BITWISE_OR(ErrorFlags)


// an error message from the typechecker; I plan to expand
// this to contain lots of information about the error, but
// for now it's just a string like every other typechecker
// produces
class ErrorMsg {
public:
  SourceLoc loc;          // where the error happened
  string msg;             // english explanation
  ErrorFlags flags;       // various

public:
  ErrorMsg(SourceLoc L, char const *m, ErrorFlags f)
    : loc(L), msg(m), flags(f) {}
  ~ErrorMsg();

  bool isWarning() const { return !!(flags & EF_WARNING); }
  bool disambiguates() const { return !!(flags & EF_DISAMBIGUATES); }

  string toString() const;
};


// simple interface for reporting errors, so I can break a dependency
// cycle
#if 0
class ErrorReporter {
public:
  virtual void reportError(char const *msg)=0;
};
#endif // 0













#if 0

// each kind of semantic error has its own code; this enum is
// declared outside SemanticError to reduce verbosity of naming
// the codes (i.e. no "SemanticError::" needed)
enum SemanticErrorCode {
  SE_DUPLICATE_VAR_DECL,
    // duplicate variable declaration
    //   varName: the variable declared more than once

  SE_UNDECLARED_VAR,
    // use of an undeclared variable
    //   varName: the name that wasn't declared

  SE_GENERAL,
    // error not more precisely classified
    //   msg: describes the problem

  SE_INTERNAL_ERROR,
    // internal parser error
    //   msg: describes problem

  NUM_CODES
};


// semantic error object
class SemanticError {
public:     // data
  // node where the error was detected; useful for printing
  // location in input file
  CCTreeNode const *node;

  // what is wrong, in a general sense
  SemanticErrorCode code;

  // code-specific fields
  string msg;                 // some general message
  string varName;             // name of a relevant variable

public:     // funcs
  // construct a blank error object; usually additional fields
  // should then be filled-in
  SemanticError(CCTreeNode const *node, SemanticErrorCode code);

  SemanticError(SemanticError const &obj);
  ~SemanticError();

  SemanticError& operator= (SemanticError const &obj);

  // combine the various data into a single string explaining
  // the problem and where it occurred
  string whyStr() const;
};


// exception object to carry a SemanticError
class XSemanticError : public xBase {
public:     // data
  SemanticError err;

public:     // funcs
  XSemanticError(SemanticError const &err);
  XSemanticError(XSemanticError const &obj);
  ~XSemanticError();
};

#endif // 0

#endif // CC_ERR_H
