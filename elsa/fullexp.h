// fullexp.h            see license.txt for copyright and terms of use
// FullExpressionAnnot, an object used by elaboration to keep
// track of what must be destroyed at the end of a "full expression"

#ifndef FULLEXP_H
#define FULLEXP_H

// NOTE: This file is #included by cc.ast when cc_elaborate.ast is active.

#include "astlist.h"      // ASTList

class Declaration;        // cc.ast

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

  bool noTemporaries() const { return declarations.isEmpty(); }
};

#endif // FULLEXP_H
