// cc_elaborate.h            see license.txt for copyright and terms of use
// info for an elaboration pass (see also cc_elaborate.ast)

#ifndef CC_ELABORATE_H
#define CC_ELABORATE_H

#include "astlist.h"      // ASTList

class Declaration;        // cc.ast
class Scope;              // cc_scope.h
class Env;                // cc_env.h

// Objects with a FullExpressionAnnot have their own scope containing
// both these declarations and their sub-expressions.  The
// declarations come semantically before the sub-expression with
// which this object is associated.
class FullExpressionAnnot {
public:      // data
  ASTList<Declaration> declarations;

public:      // funcs
  FullExpressionAnnot();
  ~FullExpressionAnnot();

  // defined in cc_tcheck.cc (for now)
  Scope *tcheck_preorder(Env &env);
  void tcheck_postorder(Env &env, Scope *scope);
};


#endif // CC_ELABORATE_H
