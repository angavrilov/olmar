// cil.cc
// code for cil.h

// TODO9: virtually all of this code could have been
// automatically generated from cil.h .. so, write a
// tool to do that (or a language extension, or ..)

#include "cil.h"        // this module
#include "cc_type.h"    // Type, etc.
#include "cc_env.h"     // Env
#include "macros.h"     // STATICASSERT

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
  // little hack: let the NULL get all the way here before
  // dealing with it
  if (this == NULL) {
    return env->getSimpleType(ST_VOID);
  }

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


STATICDEF void CilExpr::printAllocStats(bool anyway)
{
  if (anyway || numAllocd != 0) {
    cout << "cil expr nodes: " << numAllocd
         << ", max  nodes: " << maxAllocd
         << endl;
  }
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
  // must be an lvalue
  xassert(!var->isEnumValue());

  CilLval *ret = new CilLval(CilLval::T_VARREF);
  ret->varref.var = var;
  return ret;
}

CilExpr *newVarRefExpr(Variable *var)
{
  if (var->isEnumValue()) {
    // since Cil doesn't have enums, instead generate a literal
    return newIntLit(var->enumValue);
  }
  else {
    // usual case
    return newVarRef(var);
  }
}


CilLval *newDeref(CilExpr *ptr)
{
  CilLval *ret = new CilLval(CilLval::T_DEREF);
  ret->deref.addr = ptr;
  return ret;
}

CilLval *newFieldRef(CilLval *record, Variable *field)
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
  memset(&assign, 0, sizeof(assign));

  // init some fields
  switch (itag) {
    case T_CALL:
      call = (CilFnCall*)this;
      break;

    INCL_SWITCH
  }
}


CilInst::~CilInst()
{
  switch (itag) {
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

    case NUM_ITAGS:
      xfailure("bad tag");
  }

  numAllocd--;
}


STATICDEF void CilInst::validate(ITag tag)
{
  xassert(0 <= tag && tag < NUM_ITAGS);
}


STATICDEF void CilInst::printAllocStats(bool anyway)
{                                                  
  if (anyway || numAllocd != 0) {
    cout << "cil inst nodes: " << numAllocd
         << ", max  nodes: " << maxAllocd
         << endl;
  }
}


CilExpr *nullableCloneExpr(CilExpr *expr)
{
  return expr? expr->clone() : NULL;
}


CilInst *CilInst::clone() const
{
  // note: every owner pointer must be cloned, not
  // merely copied!
  switch (itag) {
    case NUM_ITAGS:
      xfailure("bad tag");

    case T_ASSIGN:
      return newAssignInst(assign.lval->clone(), assign.expr->clone());

    case T_CALL:
      return call->clone();
  }

  xfailure("bad tag");
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
  if (itag == T_CALL) {
    call->printTree(ind, os);
    return;
  }

  indent(ind, os);

  switch (itag) {
    case T_CALL:
      // handled above already; not reached

    case T_ASSIGN:
      os << "assign " << assign.lval->toString()
         << " := " << assign.expr->toString() << " ;\n";
      break;

    case NUM_ITAGS:
      xfailure("bad tag");
  }
}


CilInst *newAssignInst(CilLval *lval, CilExpr *expr)
{
  xassert(lval && expr);
  CilInst *ret = new CilInst(CilInst::T_ASSIGN);
  xassert(lval->isLval());    // stab in the dark ..
  ret->assign.lval = lval;
  ret->assign.expr = expr;
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

  if (result) {
    delete result;
  }
  delete func;
  // args automatically deleted
}


CilLval *nullableCloneLval(CilLval *src)
{
  if (src) {
    return src->clone();
  }
  else {
    return NULL;
  }
}


CilFnCall *CilFnCall::clone() const
{
  CilFnCall *ret = new CilFnCall(nullableCloneLval(result),
                                 func->clone());

  FOREACH_OBJLIST(CilExpr, args, iter) {
    ret->args.append(iter.data()->clone());
  }

  return ret;
}


void CilFnCall::printTree(int ind, ostream &os) const
{
  indent(ind, os);
  os << "call ";
  if (result) {
    os << result->toString() << " ";
  }
  os << ":= " << func->toString()
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


// ------------------ CilStmt ---------------
int CilStmt::numAllocd = 0;
int CilStmt::maxAllocd = 0;

CilStmt::CilStmt(STag tag)
  : stag(tag)
{
  INC_HIGH_WATER(numAllocd, maxAllocd);

  validate(stag);

  // clear fields; 'ifthenelse' is the largest in
  // the union
  memset(&ifthenelse, 0, sizeof(ifthenelse));

  // experiment ... statically verify the assumption
  // that 'ifthenelse' is really the biggest
  STATIC_ASSERT((char*)(this + 1) - (char*)&ifthenelse == sizeof(ifthenelse));

  // init some fields
  switch (stag) {
    case T_COMPOUND:
      comp = (CilCompound*)this;
      break;

    INCL_SWITCH
  }
}


CilStmt::~CilStmt()
{
  switch (stag) {
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

    case T_SWITCH:
      delete switchStmt.expr;
      delete switchStmt.body;
      break;

    case T_CASE:
      break;

    case T_DEFAULT:
      break;

    case T_INST:
      delete inst.inst;
      break;  

    case NUM_STAGS:
      xfailure("bad tag");
  }

  numAllocd--;
}


STATICDEF void CilStmt::validate(STag tag)
{
  xassert(0 <= tag && tag < NUM_STAGS);
}


STATICDEF void CilStmt::printAllocStats(bool anyway)
{
  if (anyway || numAllocd != 0) {
    cout << "cil stmt nodes: " << numAllocd
         << ", max  nodes: " << maxAllocd
         << endl;
  }
}


CilStmt *CilStmt::clone() const
{
  // note: every owner pointer must be cloned, not
  // merely copied!
  switch (stag) {
    case NUM_STAGS:
      xfailure("bad tag");

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
      return newReturn(nullableCloneExpr(ret.expr));

    case T_SWITCH:
      return newSwitch(switchStmt.expr->clone(),
                       switchStmt.body->clone());

    case T_CASE:
      return newCase(caseStmt.value);

    case T_INST:
      return newInst(inst.inst->clone());  

    case T_DEFAULT:
      return newDefault();
  }

  xfailure("bad tag");
}


void CilStmt::printTree(int ind, ostream &os) const
{
  if (stag == T_COMPOUND) {
    comp->printTree(ind, os);
    return;
  }
  else if (stag == T_INST) {
    inst.inst->printTree(ind, os);
    return;
  }

  indent(ind, os);

  switch (stag) {
    case T_COMPOUND:
    case T_INST:
      // handled above already; not reached

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

    case T_SWITCH:
      os << "switch (" << switchStmt.expr->toString() << ") {\n";
      switchStmt.body->printTree(ind+2, os);
      indent(ind, os) << "}\n";
      break;

    case T_CASE:
      os << "case " << caseStmt.value << ":\n";
      break;

    case T_DEFAULT:
      os << "default:\n";
      break;

    case NUM_STAGS:
      xfailure("bad tag");
  }
}


CilStmt *newWhileLoop(CilExpr *expr, CilStmt *body)
{
  xassert(expr && body);
  CilStmt *ret = new CilStmt(CilStmt::T_LOOP);
  ret->loop.cond = expr;
  ret->loop.body = body;
  return ret;
}

CilStmt *newIfThenElse(CilExpr *cond, CilStmt *thenBranch, CilStmt *elseBranch)
{
  xassert(cond && thenBranch && elseBranch);
  CilStmt *ret = new CilStmt(CilStmt::T_IFTHENELSE);
  ret->ifthenelse.cond = cond;
  ret->ifthenelse.thenBr = thenBranch;
  ret->ifthenelse.elseBr = elseBranch;
  return ret;
}

CilStmt *newLabel(LabelName label)
{
  CilStmt *ret = new CilStmt(CilStmt::T_LABEL);
  ret->label.name = new LabelName(label);
  return ret;
}

CilStmt *newGoto(LabelName label)
{
  CilStmt *ret = new CilStmt(CilStmt::T_JUMP);
  ret->jump.dest = new LabelName(label);
  return ret;
}

CilStmt *newReturn(CilExpr *expr /*nullable*/)
{
  CilStmt *ret = new CilStmt(CilStmt::T_RET);
  ret->ret.expr = expr;
  return ret;
}

CilStmt *newSwitch(CilExpr *expr, CilStmt *body)
{
  xassert(expr && body);
  CilStmt *ret = new CilStmt(CilStmt::T_SWITCH);
  ret->switchStmt.expr = expr;
  ret->switchStmt.body = body;
  return ret;
}

CilStmt *newCase(int val)
{
  CilStmt *ret = new CilStmt(CilStmt::T_CASE);
  ret->caseStmt.value = val;
  return ret;
}

CilStmt *newDefault()
{
  CilStmt *ret = new CilStmt(CilStmt::T_DEFAULT);
  return ret;
}

CilStmt *newInst(CilInst *inst)
{  
  xassert(inst);
  CilStmt *ret = new CilStmt(CilStmt::T_INST);
  ret->inst.inst = inst;
  return ret;
}

CilStmt *newAssign(CilLval *lval, CilExpr *expr)
{
  return newInst(newAssignInst(lval, expr));
}


// ------------------- CilStatements ---------------------
CilStatements::CilStatements()
{}

CilStatements::~CilStatements()
{}


void CilStatements::append(CilStmt *inst)
{
  stmts.append(inst);
}


// the point of separating this from CilCompound::printTree
// is to get a version which doesn't wrap braces around the
// code; this is useful in cc.gr where I am artificially
// creating compounds but they're all really at the same scope
// (namely toplevel)
void CilStatements::printTreeNoBraces(int ind, ostream &os) const
{
  FOREACH_OBJLIST(CilStmt, stmts, iter) {
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
  : CilStmt(CilStmt::T_COMPOUND)
{}

CilCompound::~CilCompound()
{
  comp = NULL;          // prevent infinite recursion
}


CilCompound *CilCompound::clone() const
{
  CilCompound *ret = new CilCompound();

  FOREACH_OBJLIST(CilStmt, stmts, iter) {
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


// -------------------- CilFnDefn -----------------
CilFnDefn::~CilFnDefn()
{}


void CilFnDefn::printTree(int ind, ostream &os) const
{
  os << "fundecl " << var->name
     << " : " << var->type->toString();
  indent(ind, os) << " {\n";

  // print locals
  SFOREACH_OBJLIST(Variable, locals, iter) {
    indent(ind+2, os)
       << "local " << iter.data()->name
       << " : " << iter.data()->type->toString() << " ;\n";
  }
  os << endl;

  // print statements
  bodyStmt.printTree(ind+2, os);

  indent(ind, os) << "}\n";
}


// ------------------ CilProgram ---------------
CilProgram::CilProgram()
{}

CilProgram::~CilProgram()
{}


void CilProgram::printTree(int ind, ostream &os) const
{
  SFOREACH_OBJLIST(Variable, globals, iter) {
    indent(ind, os)
       << "global " << iter.data()->name
       << " : " << iter.data()->type->toString() << " ;\n";
  }
  
  FOREACH_OBJLIST(CilFnDefn, funcs, iter) {
    iter.data()->printTree(ind, os);
  }
}


void CilProgram::empty()
{
  globals.removeAll();
  funcs.deleteAll();
}


// --------------------- CilContext -----------------
void CilContext::append(CilStmt * /*owner*/ s) const
{
  if (!isTrial) {
    stmts->append(s);
  }
  else {
    delete s;
  }
}


void CilContext::addVarDecl(Variable *var) const
{
  if (!isTrial) {
    if (fn) {
      fn->locals.append(var);
    }
    else {
      // it's a global if we're not in the context
      // of any function definition
      prog->globals.append(var);
    }
  }
}

