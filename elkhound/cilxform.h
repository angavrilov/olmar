// cilxform.h
// fairly general transformation interface, based
// on something like the "visitor pattern"

#ifndef CILXFORM_H
#define CILXFORM_H

#include "cil.h"     // underlying data structures

// a client wishing to traverse and modify a Cil
// structure derives a new class from this one, then
// overrides the methods as needed
class CilXform { 
public:   // data
  // a somewhat ugly detail is, if we need to ask about the type
  // of anything, an environment is needed (so we can construct
  // types ...); so all xforms will carry one
  Env * const env;

public:   // funcs
  CilXform(Env *e) : env(e) {}
  virtual ~CilXform();

  // the callTransformXXX methods call the (overridden)
  // transformXXX methods, and implement the transformation
  // semantics the latter methods expect of their caller
  // (e.g., replacing references and deleting old versions)

  #define DECL_TRANSFORM(name,type)               \
    virtual void callTransform##name(type &arg);  \
    virtual type transform##name(type arg);

  // for scalars, we just do replacement
  DECL_TRANSFORM(Int, int)
  DECL_TRANSFORM(UnaryOp, UnaryOp)
  DECL_TRANSFORM(BinOp, BinOp)

  // for owner pointers, we deallocate old versions
  // and replace the pointer values with the new value
  DECL_TRANSFORM(Expr, CilExpr*)
  DECL_TRANSFORM(Lval, CilLval*)
  DECL_TRANSFORM(Offset, CilOffset*)
  DECL_TRANSFORM(Label, LabelName*)
  DECL_TRANSFORM(Inst, CilInst*)
  DECL_TRANSFORM(Stmt, CilStmt*)
  DECL_TRANSFORM(OwnerVar, Variable *)
  DECL_TRANSFORM(FieldInit, FieldInit *)

  // for serf pointers, we replace but do not deallocate
  DECL_TRANSFORM(Type, Type const *)
  DECL_TRANSFORM(CompoundType, CompoundType const *)
  DECL_TRANSFORM(Var, Variable *)

  // here, I redefine this macro for client use; the idea
  // is a client will copy+paste the above decls, remove
  // the ones they don't want, and implement the rest
  #undef DECL_TRANSFORM
  #define DECL_TRANSFORM(name,type)               \
    virtual type transform##name(type arg);
};


// little bit of syntactic sugar, for when a transformer
// is ready to recurse on the current structure
#define RECURSE_XFORM(src) \
  src->xform(*this);


#endif // CILXFORM_H
