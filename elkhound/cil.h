// cil.h
// C Intermediate Language

#ifndef CIL_H
#define CIL_H

#include "str.h"       // string

// other files
class Type;
class FunctionType;

// fwd
class CilExpr;
class CilLval;

// ------------- names --------------
typedef string VarName;
VarName newTmpVar();

typedef string LabelName;
LabelName newTmpLabel();


// -------------- CilExpr ------------
class CilExpr {};
Type const *typeOf(CilExpr *expr);

CilExpr *newIntLit(int val);

enum BinOp { OP_PLUS, OP_MINUS, OP_TIMES, OP_DIVIDE,
             OP_MOD, OP_LSHIFT, OP_RSHIFT,
             OP_LT, OP_GT, OP_LTE, OP_GTE,
             OP_EQUAL, OP_NOTEQUAL,
             OP_BITAND, OP_BITXOR, OP_BITOR,
             OP_AND, OP_OR };
CilExpr *newBinExpr(BinOp op, CilExpr *e1, CilExpr *e2);

CilExpr *newAddrOfExpr(CilLval *lval);

enum UnaryOp { OP_NEGATE, OP_NOT, OP_BITNOT };
CilExpr *newUnaryExpr(UnaryOp op, CilExpr *expr);

CilExpr *newCastExpr(Type const *type, CilExpr *expr);


// ------------------- CilLval -------------------
class CilLval : public CilExpr {};
CilLval *asLval(CilExpr *expr);
bool isLval(CilExpr *expr);

CilLval *newCastLval(Type const *type, CilExpr *expr);
CilLval *newFieldRef(CilExpr *record, VarName field);
CilLval *newDeref(CilExpr *ptr);
CilLval *newArrayAccess(CilExpr *array, CilExpr *index);
CilLval *newVarRef(VarName var);


// ---------------------- CilInst ----------------
class CilInst {};
CilInst *newVarDecl(VarName var, Type const *type);
CilInst *newAssign(CilLval *lval, CilExpr *expr);
CilInst *newLabel(LabelName label);
CilInst *newGoto(LabelName label);
CilInst *newWhileLoop(CilExpr *expr, CilInst *body);
CilInst *newReturn(CilExpr *expr /*nullable*/);
CilInst *newFunDecl(VarName name, FunctionType const *type, CilInst *body);
CilInst *newFreeInst(CilExpr *ptr);
CilInst *newIfThenElse(CilExpr *cond, CilInst *thenBranch, CilInst *elseBranch);


class CilFnCall : public CilInst {
public:
  void appendArg(CilExpr *arg);
};
CilFnCall *newFnCall(CilLval *result, CilExpr *fn);    // args appended later

class CilInstructions {
public:
  void append(CilInst *inst);
};

class CilCompound : public CilInst, public CilInstructions {
public:                
  // deep copy
  CilCompound *clone() const;

  void printTree() const;
};
CilCompound *newCompound();

#endif // CIL_H
