// newlval.cc
// code for newlval.h

#include "newlval.h"    // this module
                        

// handy syntactic sugar
#define X (src->extra())

CilLval *NewLvalXform::transformLval(CilLval *src)
{
  // FieldRef(CastL(type, lval), f) -> error
  // because 'type' would have to be a struct, but lval
  // cast to a struct isn't allowed
  if (src->ltag == CilLval::T_FIELDREF &&
      src->fieldref.record->ltag == CilLval::T_CASTL) {
    cout << "illegal fieldref of castl at " << src->locString() << endl;
    return src;
  }

  RECURSE_XFORM(src);

  // Varref(v) -> VarOfs(v, [])
  if (src->ltag == CilLval::T_VARREF) {
    return newVarOfs(X, src->varref.var, NULL);
  }

  // Deref(e) -> DerefOfs(e, [])
  if (src->ltag == CilLval::T_DEREF) {
    return newDerefOfs(X, xfr(src->deref.addr), NULL);
  }

  // TODO: these are wrong because they reverse the
  // order of the fields .. it's easy to deal with
  // in my repr., but not in the ML repr, which could
  // mean there's a deeper problem with that repr..
  //   FieldRef(VarOfs(v, ofs), f) -> VarOfs(v, Field(f, ofs))
  //   FieldRef(DerefOfs(e, ofs), f) -> DerefOfs(e, Field(f, ofs))
  if (src->ltag == CilLval::T_FIELDREF) {
    CilLval *ret = xfr(src->fieldref.record);
    xassert(ret->ltag == CilLval::T_VAROFS ||
            ret->ltag == CilLval::T_DEREFOFS);
    ret->ofs.offsets->append(new CilOffset(src->fieldref.field));
    return ret;
  }

  // ArrayElt(a, i) -> DerefOfs(a, [Index(i)])
  if (src->ltag == CilLval::T_ARRAYELT) {
    OffsetList *ofs = new OffsetList;
    ofs->append( new CilOffset(xfr(src->arrayelt.index)) );
    return newDerefOfs(X, xfr(src->arrayelt.array), ofs);
  }

  // TODO: (essentially a simplification transformation)
  // Deref(BinExp('+', e1, e2)) -> DerefOfs(e1, IndexOfs(e2, NoOffset))
  // Deref(BinExp('-', e1, e2)) -> DerefOfs(e1, IndexOfs(UnExp('-', e2), NoOffset))
  // for both, type of e1 must be ptr, type of e2 must be int

  if (!( src->ltag == CilLval::T_VAROFS ||
         src->ltag == CilLval::T_DEREFOFS )) {
    cout << "unconverted old lval at " << src->locString() << endl;
  }

  return src;
}


CilExpr *NewLvalXform::transformExpr(CilExpr *src)
{
  // Lval(CastL(type, lval)) -> CastE(type, Lval(lval))
  if (src->etag == CilExpr::T_LVAL &&
      src->lval->ltag == CilLval::T_CASTL) {
    return newCastExpr(X, src->lval->castl.type,
                          newLvalExpr(X, xfr(src->lval->castl.lval)));
  }

  RECURSE_XFORM(src);
  return src;
}


CilInst *NewLvalXform::transformInst(CilInst *src)
{
  // Set(CastL(type, lval), expr) -> Set(lval, CastE(lvalType, expr))
  while (src->itag == CilInst::T_ASSIGN &&
         src->assign.lval->ltag == CilLval::T_CASTL) {
    CilLval *lval = xfr(src->assign.lval->castl.lval);
    Type const *lvalType = lval->getType(env);
    delete src->assign.lval;

    CilExpr *expr = xfr(src->assign.expr);

    src->assign.lval = lval;
    src->assign.expr = newCastExpr(X, lvalType, expr);
  }

  RECURSE_XFORM(src);
  return src;
}
