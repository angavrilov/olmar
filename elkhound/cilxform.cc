// cilxform.cc
// skeleton traversal stubs for cilxform.h

#include "cilxform.h"     // this module


// dummy destructor so gcc will shut the hell up
CilXform::~CilXform()
{}


#define DEFN_SCALAR_CALL_TRANSFORM(name,type)      \
  void CilXform::callTransform##name(type &arg)    \
  {                                                \
    arg = transform##name(arg);                    \
  }

#define DEFN_SCALAR_DO_TRANSFORM(name,type)        \
  type CilXform::transform##name(type arg)         \
  {                                                \
    return arg;                                    \
  }

#define DEFN_SCALAR_TRANSFORM(name,type)   \
  DEFN_SCALAR_CALL_TRANSFORM(name,type)    \
  DEFN_SCALAR_DO_TRANSFORM(name,type)

DEFN_SCALAR_TRANSFORM(Int, int)
DEFN_SCALAR_TRANSFORM(UnaryOp, UnaryOp)
DEFN_SCALAR_TRANSFORM(BinOp, BinOp)
DEFN_SCALAR_TRANSFORM(Type, Type const *)
DEFN_SCALAR_TRANSFORM(CompoundType, CompoundType const *)
DEFN_SCALAR_TRANSFORM(Var, Variable *)


#define DEFN_OWNER_CALL_TRANSFORM(name,type)         \
  void CilXform::callTransform##name(type &arg)      \
  {                                                  \
    type newArg = transform##name(arg);              \
    if (newArg != arg) {                             \
      delete arg;      /* owner: deallocate */       \
      arg = newArg;    /* replace */                 \
    }                                                \
  }

#define DEFN_OWNER_DO_TRANSFORM(name,type)           \
  type CilXform::transform##name(type arg)           \
  {                                                  \
    RECURSE_XFORM(arg);                              \
    return arg;                                      \
  }

#define DEFN_OWNER_TRANSFORM(name,type)   \
  DEFN_OWNER_CALL_TRANSFORM(name,type)    \
  DEFN_OWNER_DO_TRANSFORM(name,type)


DEFN_OWNER_TRANSFORM(Expr, CilExpr*)
DEFN_OWNER_TRANSFORM(Lval, CilLval*)
DEFN_OWNER_TRANSFORM(Offset, CilOffset*)
DEFN_OWNER_TRANSFORM(Inst, CilInst*)
DEFN_OWNER_TRANSFORM(Stmt, CilStmt*)
DEFN_OWNER_TRANSFORM(FieldInit, FieldInit*)

DEFN_OWNER_CALL_TRANSFORM(OwnerVar, Variable *)
DEFN_SCALAR_DO_TRANSFORM(OwnerVar, Variable *)

DEFN_OWNER_CALL_TRANSFORM(Label, LabelName*)
DEFN_SCALAR_DO_TRANSFORM(Label, LabelName*)
