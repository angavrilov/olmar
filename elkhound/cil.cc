// cil.cc
// code for cil.h

#include "cil.h"        // this module
#include "cc_type.h"    // Type, etc.

#include <stdlib.h>     // rand


// ------------- names --------------
VarName newTmpVar()
{
  return stringc << "tmpvar" << rand();
}

LabelName newTmpLabel()
{
  return stringc << "tmplabel" << rand();
}


// -------------- CilExpr ------------
Type const *typeOf(CilExpr *expr) { return NULL; }

CilExpr *newIntLit(int val) { return NULL; }

CilExpr *newBinExpr(BinOp op, CilExpr *e1, CilExpr *e2) { return NULL; }

CilExpr *newAddrOfExpr(CilLval *lval) { return NULL; }

CilExpr *newUnaryExpr(UnaryOp op, CilExpr *expr) { return NULL; }

CilExpr *newCastExpr(Type const *type, CilExpr *expr) { return NULL; }


// ------------------- CilLval -------------------
CilLval *asLval(CilExpr *expr) { return NULL; }
bool isLval(CilExpr *expr) { return false; }

CilLval *newCastLval(Type const *type, CilExpr *expr) { return NULL; }
CilLval *newFieldRef(CilExpr *record, VarName field) { return NULL; }
CilLval *newDeref(CilExpr *ptr) { return NULL; }
CilLval *newArrayAccess(CilExpr *array, CilExpr *index) { return NULL; }
CilLval *newVarRef(VarName var) { return NULL; }


// ---------------------- CilInst ----------------
CilInst *newVarDecl(VarName var, Type const *type) { return NULL; }
CilInst *newAssign(CilLval *lval, CilExpr *expr) { return NULL; }
CilInst *newLabel(LabelName label) { return NULL; }
CilInst *newGoto(LabelName label) { return NULL; }
CilInst *newWhileLoop(CilExpr *expr, CilInst *body) { return NULL; }
CilInst *newReturn(CilExpr *expr /*nullable*/) { return NULL; }
CilInst *newFunDecl(VarName name, FunctionType const *type, CilInst *body) { return NULL; }
CilInst *newFreeInst(CilExpr *ptr) { return NULL; }
CilInst *newIfThenElse(CilExpr *cond, CilInst *thenBranch, CilInst *elseBranch) { return NULL; }


void CilFnCall::appendArg(CilExpr *arg) {}
CilFnCall *newFnCall(CilLval *result, CilExpr *fn) { return NULL; }    // args appended later

void CilInstructions::append(CilInst *inst) {}

CilCompound *CilCompound::clone() const { return NULL; }
void CilCompound::printTree() const {}

CilCompound *newCompound() { return NULL; }
