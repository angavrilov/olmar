// cil.cc
// code for cil.h

// TODO9: virtually all of this code could have been
// automatically generated from cil.h .. so, write a
// tool to do that (or a language extension, or ..)

#include "cil.h"        // this module
#include "cc_type.h"    // Type, etc.
#include "cc_env.h"     // Env
#include "macros.h"     // STATICASSERT
#include "cc_tree.h"    // CCTreeNode

#include <stdlib.h>     // rand
#include <string.h>     // memset


// ------------- names --------------
LabelName newTmpLabel()
{
  // TODO2: fix this sorry hack
  return stringc << "tmplabel" << (rand() % 1000);
}


// ------------ CilThing --------------
string CilThing::locString() const
{  
  return treeNode->locString();
}

string CilThing::locComment() const
{                                  
  return stringc << "                  // " << locString() << "\n";
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

CilExpr::CilExpr(CCTreeNode *tn, ETag t)
  : CilThing(tn),
    etag(t)
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
      return newIntLit(treeNode, lit.value);

    case T_LVAL:
      return lval->clone();

    case T_UNOP:
      return newUnaryExpr(treeNode, unop.op, unop.exp->clone());

    case T_BINOP:
      return newBinExpr(treeNode,
                        binop.op,
                        binop.left->clone(),
                        binop.right->clone());

    case T_CASTE:
      return newCastExpr(treeNode, caste.type, caste.exp->clone());

    case T_ADDROF:
      return newAddrOfExpr(treeNode, addrof.lval->clone());
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
      return stringc << "([" << caste.type->toString(0)
                     << "] " << caste.exp->toString() << ")";
    case T_ADDROF:
      return stringc << "(& " << addrof.lval->toString() << ")";
  }
}


CilExpr *newIntLit(CCTreeNode *tn, int val)
{
  CilExpr *ret = new CilExpr(tn, CilExpr::T_LITERAL);
  ret->lit.value = val;
  return ret;
}

CilExpr *newUnaryExpr(CCTreeNode *tn, UnaryOp op, CilExpr *expr)
{
  CilExpr *ret = new CilExpr(tn, CilExpr::T_UNOP);
  ret->unop.op = op;
  ret->unop.exp = expr;
  return ret;
}

CilExpr *newBinExpr(CCTreeNode *tn, BinOp op, CilExpr *e1, CilExpr *e2)
{
  CilExpr *ret = new CilExpr(tn, CilExpr::T_BINOP);
  ret->binop.op = op;
  ret->binop.left = e1;
  ret->binop.right = e2;
  return ret;
}

CilExpr *newCastExpr(CCTreeNode *tn, Type const *type, CilExpr *expr)
{
  CilExpr *ret = new CilExpr(tn, CilExpr::T_CASTE);
  ret->caste.type = type;
  ret->caste.exp = expr;
  return ret;
}

CilExpr *newAddrOfExpr(CCTreeNode *tn, CilLval *lval)
{
  CilExpr *ret = new CilExpr(tn, CilExpr::T_ADDROF);
  ret->addrof.lval = lval;
  return ret;
}


// ------------------- CilLval -------------------
CilLval::CilLval(CCTreeNode *tn, LTag tag)
  : CilExpr(tn, CilExpr::T_LVAL),
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
      return newVarRef(treeNode, varref.var);

    case T_DEREF:
      return newDeref(treeNode, deref.addr->clone());

    case T_FIELDREF:
      return newFieldRef(treeNode, 
                         fieldref.record->clone(), 
                         fieldref.field,
                         fieldref.recType);

    case T_CASTL:
      return newCastLval(treeNode,
                         castl.type,
                         castl.lval->clone());

    case T_ARRAYELT:
      return newArrayAccess(treeNode,
                            arrayelt.array->clone(),
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
                     << " /""*" << fieldref.recType->name << "*/"    // odd syntax to avoid irritating emacs' highlighting
                     << " . " << fieldref.field->name << ")";
    case T_CASTL:
      return stringc << "(@[" << castl.type->toString(0)
                     << "] " << castl.lval->toString() << ")";
    case T_ARRAYELT:
      return stringc << "(" << arrayelt.array->toString()
                     << " [" << arrayelt.index->toString() << "])";
  }
}


CilLval *newVarRef(CCTreeNode *tn, Variable *var)
{
  // must be an lvalue
  xassert(!var->isEnumValue());

  CilLval *ret = new CilLval(tn, CilLval::T_VARREF);
  ret->varref.var = var;
  return ret;
}

CilExpr *newVarRefExpr(CCTreeNode *tn, Variable *var)
{
  if (var->isEnumValue()) {
    // since Cil doesn't have enums, instead generate a literal
    return newIntLit(tn, var->enumValue);
  }
  else {
    // usual case
    return newVarRef(tn, var);
  }
}


CilLval *newDeref(CCTreeNode *tn, CilExpr *ptr)
{
  CilLval *ret = new CilLval(tn, CilLval::T_DEREF);
  ret->deref.addr = ptr;
  return ret;
}

CilLval *newFieldRef(CCTreeNode *tn, CilLval *record, Variable *field,
                     CompoundType const *recType)
{
  CilLval *ret = new CilLval(tn, CilLval::T_FIELDREF);
  ret->fieldref.record = record;
  ret->fieldref.field = field;
  ret->fieldref.recType = recType;
  return ret;
}

CilLval *newCastLval(CCTreeNode *tn, Type const *type, CilLval *lval)
{
  CilLval *ret = new CilLval(tn, CilLval::T_CASTL);
  ret->castl.type = type;
  ret->castl.lval = lval;
  return ret;
}

CilLval *newArrayAccess(CCTreeNode *tn, CilExpr *array, CilExpr *index)
{
  CilLval *ret = new CilLval(tn, CilLval::T_ARRAYELT);
  ret->arrayelt.array = array;
  ret->arrayelt.index = index;
  return ret;
}


// ---------------------- CilInst ----------------
int CilInst::numAllocd = 0;
int CilInst::maxAllocd = 0;

CilInst::CilInst(CCTreeNode *tn, ITag tag)
  : CilThing(tn),
    itag(tag)
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
      return newAssignInst(treeNode,
                           assign.lval->clone(), 
                           assign.expr->clone());

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
         << " := " << assign.expr->toString() 
         << ";" << locComment();
      break;

    case NUM_ITAGS:
      xfailure("bad tag");
  }
}


CilInst *newAssignInst(CCTreeNode *tn, CilLval *lval, CilExpr *expr)
{
  xassert(lval && expr);
  CilInst *ret = new CilInst(tn, CilInst::T_ASSIGN);
  xassert(lval->isLval());    // stab in the dark ..
  ret->assign.lval = lval;
  ret->assign.expr = expr;
  return ret;
}


// -------------------- CilFnCall -------------------
CilFnCall::CilFnCall(CCTreeNode *tn, CilLval *r, CilExpr *f)
  : CilInst(tn, CilInst::T_CALL),
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
  CilFnCall *ret = 
    new CilFnCall(treeNode, nullableCloneLval(result), func->clone());

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
     << "( ";

  int ct=0;
  FOREACH_OBJLIST(CilExpr, args, iter) {
    if (++ct > 1) {
      os << " ,";
    }
    os << " " << iter.data()->toString();
  }

  os << ") ;" << locComment();
}


void CilFnCall::appendArg(CilExpr *arg)
{
  args.append(arg);
}


CilFnCall *newFnCall(CCTreeNode *tn, CilLval *result, CilExpr *fn)
{
  return new CilFnCall(tn, result, fn);
}


// ------------------ CilStmt ---------------
int CilStmt::numAllocd = 0;
int CilStmt::maxAllocd = 0;

CilStmt::CilStmt(CCTreeNode *tn, STag tag)
  : CilThing(tn),
    stag(tag)
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
      return newWhileLoop(treeNode, 
                          loop.cond->clone(), 
                          loop.body->clone());

    case T_IFTHENELSE:
      return newIfThenElse(treeNode,
                           ifthenelse.cond->clone(),
                           ifthenelse.thenBr->clone(),
                           ifthenelse.elseBr->clone());

    case T_LABEL:
      return newLabel(treeNode, *(label.name));

    case T_JUMP:
      return newGoto(treeNode, *(jump.dest));

    case T_RET:
      return newReturn(treeNode, nullableCloneExpr(ret.expr));

    case T_SWITCH:
      return newSwitch(treeNode, 
                       switchStmt.expr->clone(),
                       switchStmt.body->clone());

    case T_CASE:
      return newCase(treeNode, caseStmt.value);

    case T_INST:
      return newInst(treeNode, inst.inst->clone());

    case T_DEFAULT:
      return newDefault(treeNode);
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
      os << "while ( " << loop.cond->toString() 
         << " ) {" << locComment();
      loop.body->printTree(ind+2, os);
      indent(ind, os) << "}\n";
      break;

    case T_IFTHENELSE:
      os << "if ( " << ifthenelse.cond->toString()
         << ") {" << locComment();
      ifthenelse.thenBr->printTree(ind+2, os);
      indent(ind, os) << "}\n";
      indent(ind, os) << "else {\n";
      ifthenelse.elseBr->printTree(ind+2, os);
      indent(ind, os) << "}\n";
      break;

    case T_LABEL:
      os << "label " << *(label.name)
         << " :" << locComment();
      break;

    case T_JUMP:
      os << "goto " << *(jump.dest)
         << " ;" << locComment();
      break;

    case T_RET:
      os << "return";
      if (ret.expr) {
        os << " " << ret.expr->toString();
      }
      os << " ;" << locComment();
      break;

    case T_SWITCH:
      os << "switch (" << switchStmt.expr->toString()
         << ") {" << locComment();
      switchStmt.body->printTree(ind+2, os);
      indent(ind, os) << "}\n";
      break;

    case T_CASE:
      os << "case " << caseStmt.value 
         << ":" << locComment();
      break;

    case T_DEFAULT:
      os << "default:" << locComment();
      break;

    case NUM_STAGS:
      xfailure("bad tag");
  }
}


CilStmt *newWhileLoop(CCTreeNode *tn, CilExpr *expr, CilStmt *body)
{
  xassert(expr && body);
  CilStmt *ret = new CilStmt(tn, CilStmt::T_LOOP);
  ret->loop.cond = expr;
  ret->loop.body = body;
  return ret;
}

CilStmt *newIfThenElse(CCTreeNode *tn, CilExpr *cond, CilStmt *thenBranch, CilStmt *elseBranch)
{
  xassert(cond && thenBranch && elseBranch);
  CilStmt *ret = new CilStmt(tn, CilStmt::T_IFTHENELSE);
  ret->ifthenelse.cond = cond;
  ret->ifthenelse.thenBr = thenBranch;
  ret->ifthenelse.elseBr = elseBranch;
  return ret;
}

CilStmt *newLabel(CCTreeNode *tn, LabelName label)
{
  CilStmt *ret = new CilStmt(tn, CilStmt::T_LABEL);
  ret->label.name = new LabelName(label);
  return ret;
}

CilStmt *newGoto(CCTreeNode *tn, LabelName label)
{
  CilStmt *ret = new CilStmt(tn, CilStmt::T_JUMP);
  ret->jump.dest = new LabelName(label);
  return ret;
}

CilStmt *newReturn(CCTreeNode *tn, CilExpr *expr /*nullable*/)
{
  CilStmt *ret = new CilStmt(tn, CilStmt::T_RET);
  ret->ret.expr = expr;
  return ret;
}

CilStmt *newSwitch(CCTreeNode *tn, CilExpr *expr, CilStmt *body)
{
  xassert(expr && body);
  CilStmt *ret = new CilStmt(tn, CilStmt::T_SWITCH);
  ret->switchStmt.expr = expr;
  ret->switchStmt.body = body;
  return ret;
}

CilStmt *newCase(CCTreeNode *tn, int val)
{
  CilStmt *ret = new CilStmt(tn, CilStmt::T_CASE);
  ret->caseStmt.value = val;
  return ret;
}

CilStmt *newDefault(CCTreeNode *tn)
{
  CilStmt *ret = new CilStmt(tn, CilStmt::T_DEFAULT);
  return ret;
}

CilStmt *newInst(CCTreeNode *tn, CilInst *inst)
{
  xassert(inst);
  CilStmt *ret = new CilStmt(tn, CilStmt::T_INST);
  ret->inst.inst = inst;
  return ret;
}

CilStmt *newAssign(CCTreeNode *tn, CilLval *lval, CilExpr *expr)
{
  return newInst(tn, newAssignInst(tn, lval, expr));
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
CilCompound::CilCompound(CCTreeNode *tn)
  : CilStmt(tn, CilStmt::T_COMPOUND)
{}

CilCompound::~CilCompound()
{
  comp = NULL;          // prevent infinite recursion
}


CilCompound *CilCompound::clone() const
{
  CilCompound *ret = new CilCompound(treeNode);

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


CilCompound *newCompound(CCTreeNode *tn)
{
  return new CilCompound(tn);
}


// ------------------- CilBB ------------------
int CilBB::nextId = 0;
int CilBB::numAllocd = 0;
int CilBB::maxAllocd = 0;

CilBB::CilBB(BTag t)
  : id(nextId++),            // don't know what to do with this..
    insts(),
    btag(t)
{
  INC_HIGH_WATER(numAllocd, maxAllocd);
  validate(btag);

  // clear variant fields
  memset(&ifbb, 0, sizeof(ifbb));

  // init some fields
  if (btag == T_SWITCH) {
    switchbb = (CilBBSwitch*)this;
  }
}

CilBB::~CilBB()
{
  switch (btag) {
    case T_RETURN:
      //delete ret.expr;
      break;

    case T_IF:
      //delete ifbb.expr;
      delete ifbb.thenBB;
      delete ifbb.elseBB;
      break;

    case T_SWITCH:
      if (switchbb) {
        CilBBSwitch *ths = switchbb;
        ths->CilBBSwitch::~CilBBSwitch();     // nullifies switchbb
        numAllocd++;                          // counteract double call
      }
      break;

    case T_JUMP:
      if (jump.targetLabel) {
        delete jump.targetLabel;
      }
      break;

    default:
      xfailure("bad tag");
  }

  numAllocd--;
}


void CilBB::printTree(int ind, ostream &os) const
{
  indent(ind, os) << "basicBlock_" << id << ":\n";
  ind += 2;

  SFOREACH_OBJLIST(CilInst, insts, iter) {
    iter.data()->printTree(ind, os);
  }

  if (btag == T_SWITCH) {
    switchbb->printTree(ind, os);
    return;
  }

  indent(ind, os);
  switch (btag) {
    default:
      xfailure("bad tag");

    case T_RETURN:
      if (ret.expr) {
        os << "return " << ret.expr->toString() << ";\n";
      }
      else {
        os << "return;\n";
      }
      break;

    case T_IF:                                       
      if (ifbb.loopHint) {
        os << "while-if";
      }
      else {
        os << "if";
      }
      os << " (" << ifbb.expr->toString() << ") {\n";
      ifbb.thenBB->printTree(ind+2, os);
      indent(ind, os) << "}\n";
      indent(ind, os) << "else {\n";
      ifbb.elseBB->printTree(ind+2, os);
      indent(ind, os) << "}\n";
      break;

    case T_SWITCH:
      // handled above; not reached

    case T_JUMP:
      if (jump.nextBB) {
        os << "goto basicBlock_" << jump.nextBB->id << ";\n";
      }
      else {
        // goto whose label hasn't yet been fixed-up
        os << "incompleteGoto " << *(jump.targetLabel) << ";\n";
      }
      break;
  }
}


STATICDEF void CilBB::validate(BTag btag)
{
  xassert(0 <= btag && btag < NUM_BTAGS);
}


STATICDEF void CilBB::printAllocStats(bool anyway)
{
  if (anyway || numAllocd != 0) {
    cout << "cil bb nodes: " << numAllocd
         << ", max nodes: " << maxAllocd << endl;
  }
}


CilBB *newJumpBB(CilBB *target)
{
  CilBB *ret = new CilBB(CilBB::T_JUMP);
  ret->jump.nextBB = target;
  return ret;
}


// ------------------ BBSwitchCase -----------------
BBSwitchCase::~BBSwitchCase()
{}


// ------------------ CilBBSwitch ---------------
CilBBSwitch::CilBBSwitch(CilExpr * /*owner*/ e)
  : CilBB(T_SWITCH),
    expr(e),
    exits(),
    defaultCase(NULL)
{}

CilBBSwitch::~CilBBSwitch()
{
  // prevent infinite recursion
  switchbb = NULL;
}


void CilBBSwitch::printTree(int ind, ostream &os) const
{
  // assume I already printed the block id and the instructions
  
  indent(ind, os) << "switch (" << expr->toString() << ") {\n";
           
  FOREACH_OBJLIST(BBSwitchCase, exits, iter) {
    indent(ind+2, os) << "case " << iter.data()->value << ":\n";
    iter.data()->code->printTree(ind+4, os);
  }
  
  indent(ind+2, os) << "default:\n";
  if (defaultCase) {
    defaultCase->printTree(ind+4, os);
  }
  else {
    // defaultCase isn't supposed to be nullable, but since
    // I create it NULL, it seems prudent to handle this anyway
    indent(ind+4, os) << "// NULL defaultCase!\n";
  }
  
  indent(ind, os) << "}\n";
}


// -------------------- CilFnDefn -----------------
CilFnDefn::~CilFnDefn()
{}


void CilFnDefn::printTree(int ind, ostream &os, bool stmts) const
{
  os << "fundecl " << var->name
     << " : " << var->type->toString(0);
  indent(ind, os) << " {" << locComment();

  // print locals
  SFOREACH_OBJLIST(Variable, locals, iter) {
    indent(ind+2, os)
       << "local " << iter.data()->name
       << " : " << iter.data()->type->toString(0 /*depth*/) << " ;\n";
  }
  os << endl;

  if (stmts) {
    // print statements
    bodyStmt.printTree(ind+2, os);
  }
  else {
    // print basic blocks
    indent(ind+2, os) << "startBB " << startBB->id << ";\n";
    FOREACH_OBJLIST(CilBB, bodyBB, iter) {
      iter.data()->printTree(ind+2, os);
    }
  }

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
       << " : " << iter.data()->type->toString(0 /*depth*/) << " ;\n";
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
      xassert(!var->isGlobal());
      fn->locals.append(var);
    }
    else {
      // it's a global if we're not in the context
      // of any function definition
      xassert(var->isGlobal());
      prog->globals.append(var);
    }
  }
}

