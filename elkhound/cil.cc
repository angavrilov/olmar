// cil.cc
// code for cil.h

// TODO9: virtually all of this code could have been
// automatically generated from cil.h .. so, write a
// tool to do that (or a language extension, or ..)

#include "cil.h"        // this module
#include "cc_type.h"    // Type, etc.
#include "cc_env.h"     // Env

#include <stdlib.h>     // rand
#include <string.h>     // memset


// ------------- names --------------
LabelName newTmpLabel()
{
  // TODO2: fix this sorry hack
  return stringc << "tmplabel" << (rand() % 1000);
}


// ------------ operators -------------
char const *binOpText(BinOp op)
{ 
  char const *names[] = {
    "+", "-", "*", "/", "%",
    "<<", ">>",
    "<", ">", "<=", ">=",
    "==", "!=",
    "&", "^", "|",
    "&&", "||"
  };
  STATIC_ASSERT(TABLESIZE(names) == NUM_BINOPS);

  validate(op);
  return names[op];
}

void validate(BinOp op)
{
  xassert(0 <= op && op < NUM_BINOPS);
}


char const *unOpText(UnaryOp op)
{
  char const *names[] = {
    "-", "!", "~"
  };
  STATIC_ASSERT(TABLESIZE(names) == NUM_UNOPS);
  
  validate(op);
  return names[op];
}

void validate(UnaryOp op)
{
  xassert(0 <= op && op < NUM_UNOPS);
}


// -------------- CilExpr ------------
int CilExpr::numAllocd = 0;
int CilExpr::maxAllocd = 0;

CilExpr::CilExpr(ETag t)
  : etag(t)
{
  INC_HIGH_WATER(numAllocd, maxAllocd);

  validate(etag);

  // for definiteness, blank the union fields
  memset(&binop, 0, sizeof(binop));    // largest field

  // so some initialization
  switch (etag) {
    case T_LVAL:
      lval = (CilLval*)this;
      break;
    INCL_SWITCH
  }
}


CilExpr::~CilExpr()
{
  switch (etag) {
    case T_LVAL:
      // my masochistic tendencies show through: I do some tricks
      // to get the right dtor called, instead of just adding a
      // virtual destructor to these classes ... :)  the pile of
      // bones laid before the Altar of Efficiency grows yet higher ..
      if (lval) {
        CilLval *ths = lval;
        ths->CilLval::~CilLval();
        numAllocd++;    // counteract double call to this fn
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

    INCL_SWITCH
  }

  numAllocd--;
}


Type const *CilExpr::getType(Env *env) const
{
  switch (etag) {
    default: xfailure("bad tag");
    case T_LITERAL:    return env->getSimpleType(ST_INT);
    case T_LVAL:       return lval->getType(env);
    case T_UNOP:       return unop.exp->getType(env);
    case T_BINOP:      return binop.left->getType(env);    // TODO: check that arguments have related types??
    case T_CASTE:      return caste.type;                  // TODO: check castability?
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


STATICDEF void CilExpr::printAllocStats()
{
  cout << "cil expr nodes: " << numAllocd
       << ", max  nodes: " << maxAllocd
       << endl;
}


CilExpr *CilExpr::clone() const
{
  switch (etag) {
    default: xfailure("bad tag");

    case T_LITERAL:
      return newIntLit(lit.value);
    
    case T_LVAL:
      return lval->clone();
      
    case T_UNOP: 
      return newUnaryExpr(unop.op, unop.exp->clone());

    case T_BINOP:
      return newBinExpr(binop.op,
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
    case T_LITERAL:   return stringc << (unsigned)lit.value;
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
                     << "] " << caste.exp->toString() << ")";
    case T_ADDROF:
      return stringc << "(& " << addrof.lval->toString() << ")";
  }
}


CilExpr *newIntLit(int val)
{
  CilExpr *ret = new CilExpr(CilExpr::T_LITERAL);
  ret->lit.value = val;
  return ret;
}

CilExpr *newUnaryExpr(UnaryOp op, CilExpr *expr)
{
  CilExpr *ret = new CilExpr(CilExpr::T_UNOP);
  ret->unop.op = op;
  ret->unop.exp = expr;
  return ret;
}

CilExpr *newBinExpr(BinOp op, CilExpr *e1, CilExpr *e2)
{
  CilExpr *ret = new CilExpr(CilExpr::T_BINOP);
  ret->binop.op = op;
  ret->binop.left = e1;
  ret->binop.right = e2;
  return ret;
}

CilExpr *newCastExpr(Type const *type, CilExpr *expr)
{
  CilExpr *ret = new CilExpr(CilExpr::T_CASTE);
  ret->caste.type = type;
  ret->caste.exp = expr;
  return ret;
}

CilExpr *newAddrOfExpr(CilLval *lval)
{
  CilExpr *ret = new CilExpr(CilExpr::T_ADDROF);
  ret->addrof.lval = lval;
  return ret;
}


// ------------------- CilLval -------------------
CilLval::CilLval(LTag tag)
  : CilExpr(CilExpr::T_LVAL),
    ltag(tag)
{
  validate(ltag);
  
  // clear fields
  memset(&fieldref, 0, sizeof(fieldref));
}


CilLval::~CilLval()
{
  // the tricky thing here is when a CilLval gets destroyed, 
  // either ~CilLval *or* ~CilExpr gets called -- in the latter
  // case, ~CilExpr will in turn call ~CilLval (which, either
  // way, in turn calls ~CilExpr)
  lval = NULL;    // avoid infinite recursion when ~CilExpr is called

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
      
    INCL_SWITCH
  }
}


Type const *CilLval::getType(Env *env) const
{
  switch (ltag) {
    default: xfailure("bad tag");
    case T_VARREF:    return varref.var->type;
    case T_DEREF:     return deref.addr->getType(env)
                                       ->asPointerTypeC().atType;
    case T_FIELDREF:  return fieldref.field->type;
    case T_CASTL:     return castl.type;
    case T_ARRAYELT:  return arrayelt.array->getType(env)
                                           ->asArrayTypeC().eltType;
  }
}


STATICDEF void CilLval::validate(LTag ltag)
{
  xassert(0 <= ltag && ltag < NUM_LTAGS);
}


CilLval *CilLval::clone() const
{
  switch (ltag) {
    default: xfailure("bad tag");

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
    default: xfailure("bad tag");
    case T_VARREF:    return varref.var->name;
    case T_DEREF:
      return stringc << "(* " << deref.addr->toString() << ")";
    case T_FIELDREF:
      return stringc << "(" << fieldref.record->toString()
                     << " . " << fieldref.field->name << ")";
    case T_CASTL:
      return stringc << "(@[" << castl.type->toString()
                     << "] " << castl.lval->toString() << ")";
    case T_ARRAYELT:
      return stringc << "(" << arrayelt.array->toString()
                     << " [" << arrayelt.index->toString() << "])";
  }
}


CilLval *newVarRef(Variable *var)
{
  CilLval *ret = new CilLval(CilLval::T_VARREF);
  ret->varref.var = var;
  return ret;
}

CilLval *newDeref(CilExpr *ptr)
{
  CilLval *ret = new CilLval(CilLval::T_DEREF);
  ret->deref.addr = ptr;
  return ret;
}

CilLval *newFieldRef(CilExpr *record, Variable *field)
{
  CilLval *ret = new CilLval(CilLval::T_FIELDREF);
  ret->fieldref.record = record;
  ret->fieldref.field = field;
  return ret;
}

CilLval *newCastLval(Type const *type, CilLval *lval)
{
  CilLval *ret = new CilLval(CilLval::T_CASTL);
  ret->castl.type = type;
  ret->castl.lval = lval;
  return ret;
}

CilLval *newArrayAccess(CilExpr *array, CilExpr *index)
{
  CilLval *ret = new CilLval(CilLval::T_ARRAYELT);
  ret->arrayelt.array = array;
  ret->arrayelt.index = index;
  return ret;
}


// ---------------------- CilInst ----------------
int CilInst::numAllocd = 0;
int CilInst::maxAllocd = 0;

CilInst::CilInst(ITag tag)
  : itag(tag)
{
  INC_HIGH_WATER(numAllocd, maxAllocd);

  validate(itag);

  // clear fields
  memset(&ifthenelse, 0, sizeof(ifthenelse));

  // init some fields
  switch (itag) {
    case T_CALL:
      call = (CilFnCall*)this;
      break;

    case T_COMPOUND:
      comp = (CilCompound*)this;
      break;

    INCL_SWITCH
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
        ths->CilFnCall::~CilFnCall();
        numAllocd++;          // counteract double-call
      }
      break;

    case T_COMPOUND:
      if (comp) {
        CilCompound *ths = comp;
        ths->CilCompound::~CilCompound();
        numAllocd++;          // counteract double-call
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

    INCL_SWITCH
  }

  numAllocd--;
}


STATICDEF void CilInst::validate(ITag tag)
{
  xassert(0 <= tag && tag < NUM_ITAGS);
}


STATICDEF void CilInst::printAllocStats()
{
  cout << "cil inst nodes: " << numAllocd
       << ", max  nodes: " << maxAllocd
       << endl;
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
    default: xfailure("bad tag");

    case T_VARDECL:
      return newVarDecl(vardecl.var);

    case T_FUNDECL:
      return newFunDecl(fundecl.func, fundecl.body->clone());

    case T_ASSIGN:
      return newAssign(assign.lval->clone(), assign.expr->clone());

    case T_CALL:
      return call->clone();

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


static ostream &indent(int ind, ostream &os)
{
  while (ind--) {
    os << ' ';
  }
  return os;
}


void CilInst::printTree(int ind, ostream &os) const
{ 
  if (itag == T_COMPOUND) {
    comp->printTree(ind, os);
    return;
  }
  else if (itag == T_CALL) {
    call->printTree(ind, os);
    return;
  }

  indent(ind, os);

  switch (itag) {
    case T_VARDECL:
      os << "vardecl " << vardecl.var->name
         << " : " << vardecl.var->type->toString() << " ;\n";
      break;

    case T_FUNDECL:
      os << "fundecl " << fundecl.func->name
         << " : " << fundecl.func->type->toString();
      indent(ind, os) << " {\n";
      fundecl.body->printTree(ind+2, os);
      indent(ind, os) << "}\n";
      break;

    case T_ASSIGN:
      os << "assign " << assign.lval->toString()
         << " := " << assign.expr->toString() << " ;\n";
      break;

    case T_LOOP:
      os << "while ( " << loop.cond->toString() << " ) {\n";
      loop.body->printTree(ind+2, os);
      indent(ind, os) << "}\n";
      break;
      
    case T_IFTHENELSE:
      os << "if ( " << ifthenelse.cond->toString() << ") {\n";
      ifthenelse.thenBr->printTree(ind+2, os);
      indent(ind, os) << "}\n";
      indent(ind, os) << "else {\n";
      ifthenelse.elseBr->printTree(ind+2, os);
      indent(ind, os) << "}\n";
      break;
      
    case T_LABEL:
      os << "label " << *(label.name) << " :\n";
      break;
      
    case T_JUMP:
      os << "goto " << *(jump.dest) << " ;\n";
      break;
      
    case T_RET:
      os << "return";
      if (ret.expr) {
        os << " " << ret.expr->toString();
      }
      os << " ;\n";  
      break;
      
    INCL_SWITCH
  }
}


CilInst *newVarDecl(Variable *var)
{
  CilInst *ret = new CilInst(CilInst::T_VARDECL);
  ret->vardecl.var = var;
  return ret;
}

CilInst *newFunDecl(Variable *func, CilInst *body)
{
  CilInst *ret = new CilInst(CilInst::T_FUNDECL);
  ret->fundecl.func = func;
  ret->fundecl.body = body;
  return ret;
}

CilInst *newAssign(CilLval *lval, CilExpr *expr)
{
  CilInst *ret = new CilInst(CilInst::T_ASSIGN);
  xassert(lval->isLval());    // stab in the dark ..
  ret->assign.lval = lval;
  ret->assign.expr = expr;
  return ret;
}

CilInst *newWhileLoop(CilExpr *expr, CilInst *body)
{
  CilInst *ret = new CilInst(CilInst::T_LOOP);
  ret->loop.cond = expr;
  ret->loop.body = body;
  return ret;
}

CilInst *newIfThenElse(CilExpr *cond, CilInst *thenBranch, CilInst *elseBranch)
{
  CilInst *ret = new CilInst(CilInst::T_IFTHENELSE);
  ret->ifthenelse.cond = cond;
  ret->ifthenelse.thenBr = thenBranch;
  ret->ifthenelse.elseBr = elseBranch;
  return ret;
}

CilInst *newLabel(LabelName label)
{
  CilInst *ret = new CilInst(CilInst::T_LABEL);
  ret->label.name = new LabelName(label);
  return ret;
}

CilInst *newGoto(LabelName label)
{
  CilInst *ret = new CilInst(CilInst::T_JUMP);
  ret->jump.dest = new LabelName(label);
  return ret;
}

CilInst *newReturn(CilExpr *expr /*nullable*/)
{
  CilInst *ret = new CilInst(CilInst::T_RET);
  ret->ret.expr = expr;
  return ret;
}


// -------------------- CilFnCall -------------------
CilFnCall::CilFnCall(CilLval *r, CilExpr *f)
  : CilInst(CilInst::T_CALL),
    result(r),
    func(f),
    args()      // initially empty
{}

CilFnCall::~CilFnCall()
{
  call = NULL;          // prevent infinite recursion
  
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


void CilFnCall::printTree(int ind, ostream &os) const
{
  indent(ind, os);
  os << "call " << result->toString()
     << " := " << func->toString()
     << " withargs";

  int ct=0;
  FOREACH_OBJLIST(CilExpr, args, iter) {
    if (++ct > 1) {
      os << " ,";
    }
    os << " " << iter.data()->toString();
  }

  os << " ;\n";
}


void CilFnCall::appendArg(CilExpr *arg)
{
  args.append(arg);
}


CilFnCall *newFnCall(CilLval *result, CilExpr *fn)
{
  return new CilFnCall(result, fn);
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


// the point of separating this from CilCompound::printTree
// is to get a version which doesn't wrap braces around the
// code; this is useful in cc.gr where I am artificially
// creating compounds but they're all really at the same scope
// (namely toplevel)
void CilInstructions::printTreeNoBraces(int ind, ostream &os) const
{
  FOREACH_OBJLIST(CilInst, insts, iter) {
    try {
      iter.data()->printTree(ind, os);
    }
    catch (xBase &x) {
      os << "$$$ // error printing: " << x << endl;
    }
  }
}


// ----------------- CilCompound ---------------------
CilCompound::CilCompound()
  : CilInst(CilInst::T_COMPOUND)
{}

CilCompound::~CilCompound()
{
  comp = NULL;          // prevent infinite recursion
}


CilCompound *CilCompound::clone() const
{
  CilCompound *ret = new CilCompound();

  FOREACH_OBJLIST(CilInst, insts, iter) {
    ret->append(iter.data()->clone());
  }

  return ret;
}


void CilCompound::printTree(int ind, ostream &os) const
{                            
  // removed braces and indentation because there isn't
  // local scope in Cil, but these make it look like
  // there is, at times
  //indent(ind, os) << "{\n";
  printTreeNoBraces(ind/*+2*/, os);
  //indent(ind, os) << "}\n";
}


CilCompound *newCompound()
{
  return new CilCompound();
}

