// newlval.h
// CilXform-based transformation to replace the Lvals
// the C translator produces with the Lvals the ML
// analysis code wants to see

#ifndef NEWLVAL_H
#define NEWLVAL_H

#include "cilxform.h"      // CilXform

class NewLvalXform : public CilXform {
public:
  NewLvalXform(Env *env) : CilXform(env) {}

  // things we're interested in
  DECL_TRANSFORM(Expr, CilExpr*)
  DECL_TRANSFORM(Lval, CilLval*)
  DECL_TRANSFORM(Inst, CilInst*)
};

#endif // NEWLVAL_H
