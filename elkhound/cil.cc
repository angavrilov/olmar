// cil.cc
// code for cil.h

// TODO9: virtually all of this code could have been
// automatically generated from cil.h .. so, write a
// tool to do that (or a language extension, or ..)

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
CilExpr::CilExpr(Tag t)
  : etag(t)
{
  validate(etag);

  // for definiteness, blank the union fields
  memset(&binop, 0, sizeof(binop));    // largest field

  // so some initialization
  switch (etag) {
    case T_LVAL: 
      lval = (CilLval*)this;
      break;
  }
}

      
CilExpr::~CilExpr()
{
  switch (etag) {
    case T_LVAL:
      // my masochistic tendencies show through: I do some tricks
      // to get the right dtor called, instead of just adding a
      // virtual destructor to these classes ... :)
      if (lval) {
        lval = NULL;    // avoid infinite recursion
        this->CilLval::~CilLval();
      }
      break;

    case T_UNOP:
      delete unop.exp;
      break;

    case T_BINOP:
      delete binop.left;
      delete binop.right;
      break;

    case T_CASTE:
      delete caste.exp;
      break;

    case T_ADDROF:
      delete addrof.lval;
      break;
  }
}


Type const *CilExpr::getType(Env *env) const
{
  switch (etag) {
    default: xfailure("bad tag");
    case T_LITERAL:    return env->getSimpleType(ST_INT);
    case T_LVAL:       return lval->getType(env);
    case T_UNOP:       return exp->getType(env);
    case T_BINOP:      return left->getType(env);    // TODO: check that arguments have related types??
    case T_CASTE:      return type;                  // TODO: check castability?
    case T_ADDROF:
      return env->makePtrOperType(PO_POINTER, CV_NONE,
                                  addrof.lval->getType(env));
  }
}


CilLval *CilExpr::asLval()
{
  xassert(etag == T_LVAL);
  return lval;
}


STATICDEF void CilExpr::validate(ETag tag)
{
  xassert(0 <= tag && tag < NUM_ETAGS);
}


CilExpr *CilExpr::clone() const
{
  switch (etag) {
    case T_LITERAL:
      return newIntLit(lit.value);
    
    case T_LVAL:
      return lval->clone();
      
    case T_UNOP: 
      return newUnaryExpr(unop.op, unop.exp->clone());

    case T_BINOP:
      return newBinaryExpr(binop.op, 
                           binop.left->clone(), 
                           binop.right->clone());
                           
    case T_CASTE:
      return newCastExpr(caste.type, caste.exp->clone());
      
    case T_ADDROF:
      return newAddrOfExpr(addrof.lval->clone());
  }
}


// goal is a syntax that is easy to read (i.e. uses
// infix binary operators) but also easy to parse (so
// it heavily uses parentheses -- no prec/assoc issues)
string CilExpr::toString() const
{
  switch (etag) {
    default: xfailure("bad tag");
    case T_LITERAL:   return stringc << lit.value;
    case T_LVAL:      return lval->toString();
    case T_UNOP:
      return stringc << "(" << unOpText(unop.op) << " "
                     << unop.exp->toString() << ")";
    case T_BINOP:
      return stringc << "(" << binop.left->toString() << " "
                     << binOpText(binop.op) << " "
                     << binop.right->toString() << ")";
    case T_CASTE:
      return stringc << "([" << caste.type->toString()
                     << "] " << caste.exp->toString << ")";
    case T_ADDROF:
      return stringc << "(& " << addrof.lval->toString() << ")";
  }
}


CilExpr *newIntLit(int val)
{
  CilExpr *ret = new CilExpr(T_LITERAL);
  ret->lit.value = val;
  return ret;
}

CilExpr *newUnaryExpr(UnaryOp op, CilExpr *expr)
{
  CilExpr *ret = new CilExpr(T_UNOP);
  ret->unop.op = op;
  ret->unop.exp = expr;
  return ret;
}

CilExpr *newBinExpr(BinOp op, CilExpr *e1, CilExpr *e2)
{
  CilExpr *ret = new CilExpr(T_BINOP);
  ret->binop.op = op;
  ret->binop.left = e1;
  ret->binop.right = e2;
  return ret;
}

CilExpr *newCastExpr(Type const *type, CilExpr *expr)
{
  CilExpr *ret = new CilExpr(T_CASTE);
  ret->caste.type = type;
  ret->caste.exp = expr;
  return ret;
}

CilExpr *newAddrOfExpr(CilLval *lval)
{
  CilExpr *ret = new CilExpr(T_ADDROF);
  ret->addrof.lval = lval;
  return ret;
}


// ------------------- CilLval -------------------
CilLval::CilLval(LTag tag)
  : CilExpr(T_LVAL),
    ltag(tag)
{
  validate(ltag);
  
  // clear fields
  memset(&fieldref, 0, sizeof(fieldref));
}


CilLval::~CilLval()
{
  switch (ltag) {
    case T_DEREF:
      delete deref.addr;
      break;
      
    case T_FIELDREF:
      delete fieldref.record;
      break;
      
    case T_CASTL:
      delete castl.lval;
      break;
      
    case T_ARRAYELT:
      delete arrayelt.array;
      delete arrayelt.index;
      break;
  }
}


STATICDEF void CilLval::validate(LTag ltag)
{
  xassert(0 <= ltag && ltag < NUM_LTAGS);
}


CilLval *CilLval::clone() const
{
  switch (ltag) {
    case T_VARREF:
      return newVarRef(varref.var);
      
    case T_DEREF:
      return newDeref(deref.addr->clone());

    case T_FIELDREF:
      return newFieldRef(fieldref.record->clone(), fieldref.field);
      
    case T_CASTL:
      return newCastLval(castl.type, castl.lval->clone());
      
    case T_ARRAYELT:
      return newArrayAccess(arrayelt.array->clone(),
                            arrayelt.index->clone());
  }
}


string CilLval::toString() const
{
  switch (ltag) {
    case T_VARREF:
      





CilLval *newVarRef(Variable *var)
{
  CilLval *ret = new CilLval(T_VARREF);
  ret->varref.var = var;
  return ret;
}

CilLval *newDeref(CilExpr *ptr)
{
  CilLval *ret = new CilLval(T_DEREF);
  ret->deref.addr = ptr;
  return ret;
}

CilLval *newFieldRef(CilExpr *record, VarName field)
{
  CilLval *ret = new CilLval(T_FIELDREF);
  ret->fieldref.record = record;
  ret->fieldref.field = field;
  return ret;
}

CilLval *newCastLval(Type const *type, CilExpr *expr)
{
  CilLval *ret = new CilLval(T_CASTL);
  ret->castl.type = type;
  ret->castl.lval = expr;
  return ret;
}

CilLval *newArrayAccess(CilExpr *array, CilExpr *index)
{
  CilLval *ret = new CilLval(T_ARRAYELT);
  ret->arrayelt.array = array;
  ret->arrayelt.index = index;
  return ret;
}


// ---------------------- CilInst ----------------
CilInst::CilInst(ITag tag)
  : itag(tag)
{
  validate(itag);
  
  // clear fields
  memset(&ifthenelse, 0, sizeof(ifthenelse));
  
  // init some fields
  switch (itag) {
    case T_CALL:
      call = (CilFnCall*)this;
      break;
      
    case T_COMPOUND:
      call = (CilCompound*)this;
      break;
  }
}


CilInst::~CilInst()
{
  switch (itag) {
    case T_FUNDECL:
      delete fundecl.body;
      break;
      
    case T_ASSIGN:
      delete assign.lval;
      delete assign.expr;
      break;
      
    case T_CALL:
      if (call) { 
        CilFnCall *ths = call;
        call = NULL;          // prevent infinite recursion
        ths->CilFnCall::~CilFnCall();
      }
      break;
      
    case T_FREE:
      delete free.addr;
      break;
      
    case T_COMPOUND:
      if (comp) {
        CilCompound *ths = comp;
        comp = NULL;          // prevent infinite recursion
        ths->CilCompound::~CilCompound();
      }
      break;
      
    case T_LOOP:
      delete loop.cond;
      delete loop.body;
      break;
      
    case T_IFTHENELSE:
      delete ifthenelse.cond;
      delete ifthenelse.thenBr;
      delete ifthenelse.elseBr;
      break;
      
    case T_LABEL:
      delete label.name;
      break;
      
    case T_JUMP:
      delete jump.dest;
      break;
      
    case T_RET:
      if (ret.expr) {
        // I know 'delete' accepts NULL (and I rely on that
        // in other modules); I just like to emphasize when
        // something is nullable
        delete ret.expr;
      }                         
      break;
  }
}


STATICDEF void CilInst::validate(ITag tag)
{
  xassert(0 <= tag && tag < NUM_ITAGS);
}


CilExpr *nullableClone(CilExpr *expr)
{
  return expr? expr->clone() : NULL;
}


CilInst *CilInst::clone() const
{
  // note: every owner pointer must be cloned, not
  // merely copied!
  switch (itag) {
    case T_VARDECL:
      return newVarDecl(vardecl.var);

    case T_FUNDECL:
      return newFunDecl(fundecl.func, fundecl.body->clone());

    case T_ASSIGN:
      return newAssign(assign.lval->clone(), assign.lval->clone());

    case T_CALL:
      return call->clone();

    case T_FREE:
      return newFreeInst(free.addr->clone());

    case T_COMPOUND:
      return comp->clone();

    case T_LOOP:
      return newWhileLoop(loop.cond->clone(), loop.body->clone());

    case T_IFTHENELSE:
      return newIfThenElse(ifthenelse.cond->clone(),
                           ifthenelse.thenBr->clone(),
                           ifthenelse.elseBr->clone());

    case T_LABEL:
      return newLabel(*(label.name));

    case T_JUMP:
      return newGoto(*(jump.dest));

    case T_RET:
      return newReturn(nullableClone(ret.expr));
  }
}


CilInst *newVarDecl(Variable *var)
{
  CilInst *ret = new CilInst(T_VARDECL);
  ret->vardecl.var = var;
  return ret;
}

CilInst *newFunDecl(Variable *func, CilInst *body)
{
  CilInst *ret = new CilInst(T_FUNDECL);
  ret->fundecl.func = func;
  ret->fundecl.body = body;
  return ret;
}

CilInst *newAssign(CilLval *lval, CilExpr *expr)
{
  CilInst *ret = new CilInst(T_ASSIGN);
  ret->assign.lval = lval;
  ret->assign.expr = expr;
  return ret;
}

CilInst *newFreeInst(CilExpr *ptr)
{
  CilInst *ret = new CilInst(T_FREE);
  ret->free.addr = ptr;
  return ret;
}

CilInst *newWhileLoop(CilExpr *expr, CilInst *body)
{
  CilInst *ret = new CilInst(T_LOOP);
  ret->loop.cond = expr;
  ret->loop.body = body;
  return ret;
}

CilInst *newIfThenElse(CilExpr *cond, CilInst *thenBranch, CilInst *elseBranch)
{
  CilInst *ret = new CilInst(T_IFTHENELSE);
  ret->ifthenelse.cond = cond;
  ret->ifthenelse.thenBr = thenBranch;
  ret->ifthenelse.elseBr = elseBranch;
  return ret;
}

CilInst *newLabel(LabelName label)
{
  CilInst *ret = new CilInst(T_LABEL);
  ret->label.name = new LabelName(label);
  return ret;
}

CilInst *newGoto(LabelName label)
{
  CilInst *ret = new CilInst(T_JUMP);
  ret->jump.dest = new LabelName(label);
  return ret;
}

CilInst *newReturn(CilExpr *expr /*nullable*/)
{
  CilInst *ret = new CilInst(T_RET);
  ret->ret.expr = expr;
  return ret;
}


// -------------------- CilFnCall -------------------
CilFnCall::CilFnCall(CilLval *r, CilExpr *f)
  : CilInst(T_CALL),
    result(r),
    func(f),
    args()      // initially empty
{}

CilFnCall::~CilFnCall()
{
  delete result;
  delete func;
  // args automatically deleted
}


CilFnCall *CilFnCall::clone() const
{
  CilFnCall *ret = new CilFnCall(result->clone(), func->clone());

  FOREACH_OBJLIST(CilExpr, args, iter) {
    ret->args.append(iter.data()->clone());
  }

  return ret;
}


void CilFnCall::appendArg(CilExpr *arg)
{
  args.append(arg);
}


CilFnCall *newFnCall(CilLval *result, CilExpr *fn)
{
  return newCilFnCall(result, fn);
}


// ------------------- CilInstructions ---------------------
CilInstructions::CilInstructions()
{}

CilInstructions::~CilInstructions()
{}


void CilInstructions::append(CilInst *inst)
{
  insts.append(inst);
}


// ----------------- CilCompound ---------------------
CilCompound::CilCompound()
  : CilInst(T_COMPOUND)
{}

CilCompound::~CilCompound()
{}


CilCompound *CilCompound::clone() const
{
  CilCompound *ret = new CilCompound();
  
  FOREACH_OBJLIST(CilExpr, insts, iter) {
    ret->append(iter.data()->clone());
  }
  
  return ret;
}


void CilCompound::printTree() const {}

CilCompound *newCompound()
{
  return new CilCompound();
}

