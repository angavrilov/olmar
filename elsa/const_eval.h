// const_eval.h
// evaluate Expression AST nodes to a constant, if possible

#ifndef CONST_EVAL_H
#define CONST_EVAL_H

#include "str.h"         // string

// this is a container for the evaluation state; the actual
// evaluation methods are associated with the AST nodes,
// and declared in cc_tcheck.ast
class ConstEval {
public:      // data
  // error message if failed to evaluate
  string msg;
  
  // true if the expression's value depends on a template parameter
  bool dependent;

public:
  ConstEval();
  ~ConstEval();
                                 
  // use this to set 'dependent' to true
  bool setDependent(int &result);
};

#endif // CONST_EVAL_H
