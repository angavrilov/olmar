// cc_err.h
// objects for representing errors in C++ code

#error I do not want to use this yet

#ifndef CC_ERR_H
#define CC_ERR_H

#include "str.h"       // string
#include "exc.h"       // xBase

class CCTreeNode;      // cc_tree.h


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


#endif // CC_ERR_H
