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
#include "fileloc.h"    // SourceLocation
#include "mlvalue.h"    // ml string support stuff

#include <stdlib.h>     // rand
#include <string.h>     // memset


// ------------- names --------------
LabelName newTmpLabel()
{
  // TODO2: fix this sorry hack
  return stringc << "tmplabel" << (rand() % 1000);
}


// ------------ CilThing --------------
CilThing::CilThing(CilExtraInfo n)
{
  _loc = n;   //->loc();
}

string CilThing::locString() const
{
  SourceLocation const *l = loc();
  if (l) {
    return l->toString();
  }
  else {
    return "(?loc)";
  }
}

string CilThing::locComment() const
{
  return stringc << "                  // " << locString() << "\n";
}

SourceLocation const *CilThing::loc() const
{
  return _loc;
}


MLValue unknownMLLoc()
{
  return mlRecord3("line", mlInt(0),
                   "col", mlInt(0),
                   "file", mlString("?"));
}

string CilThing::locMLString() const
{
  SourceLocation const *l = loc();
  if (l) {
    return mlRecord3("line", mlInt(l->line),
                     "col", mlInt(l->col),
                     "file", mlString(l->fname()));
  }
  else {
    return unknownMLLoc();
  }
}


// ------------ operators -------------
BinOpInfo binOpArray[] = {
  // text, mlText,    mlTag
  {  "+",  "Plus",    0      },
  {  "-",  "Minus",   1      },
  {  "*",  "Mult",    2      },
  {  "/",  "Div",     3      },
  {  "%",  "Mod",     4      },
  {  "<<", "Shiftlt", 5      },
  {  ">>", "Shiftrt", 6      },
  {  "<",  "Lt",      7      },
  {  ">",  "Gt",      8      },
  {  "<=", "Le",      9      },
  {  ">=", "Ge",      10     },
  {  "==", "Eq",      11     },
  {  "!=", "Ne",      12     },
  {  "&",  "BAnd",    13     },
  {  "^",  "BXor",    14     },
  {  "|",  "BOr",     15     },
};

BinOpInfo const &binOp(BinOp op)
{
  STATIC_ASSERT(TABLESIZE(binOpArray) == NUM_BINOPS);

  validate(op);
  return binOpArray[op];
}

void validate(BinOp op)
{
  xassert(0 <= op && op < NUM_BINOPS);
}



UnaryOpInfo unaryOpArray[] = {
  // text, mlText, mlTag
  {  "-",  "Neg",  0       },
  {  "!",  "LNot", 1       },
  {  "~",  "BNot", 2       },
};

UnaryOpInfo const &unOp(UnaryOp op)
{
  STATIC_ASSERT(TABLESIZE(unaryOpArray) == NUM_UNOPS);

  validate(op);
  return unaryOpArray[op];
}

void validate(UnaryOp op)
{
  xassert(0 <= op && op < NUM_UNOPS);
}


// -------------- CilExpr ------------
int CilExpr::numAllocd = 0;
int CilExpr::maxAllocd = 0;

CilExpr::CilExpr(CilExtraInfo tn, ETag t)
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
      return newIntLit(extra(), lit.value);

    case T_LVAL:
      return lval->clone();

    case T_UNOP:
      return newUnaryExpr(extra(), unop.op, unop.exp->clone());

    case T_BINOP:
      return newBinExpr(extra(),
                        binop.op,
                        binop.left->clone(),
                        binop.right->clone());

    case T_CASTE:
      return newCastExpr(extra(), caste.type, caste.exp->clone());

    case T_ADDROF:
      return newAddrOfExpr(extra(), addrof.lval->clone());
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


#define MKTAG(n,t) MAKE_ML_TAG(exp, n, t)
MKTAG(0, Const)
MKTAG(1, Lval)
MKTAG(2, SizeOf)
MKTAG(3, UnOp)
MKTAG(4, BinOp)
MKTAG(5, CastE)
MKTAG(6, AddrOf)
#undef MKTAG


// goal here is a syntax that corresponds to the structure
// ML uses internally, so we can perhaps write a general-purpose
// ascii-to-ML parser (or perhaps one exists?)
string CilExpr::toMLString() const
{
  switch (etag) {
    default: xfailure("bad tag");
    case T_LITERAL:   
      return stringc << "(" << exp_Const << " "
                     <<   "(Int " << mlInt(lit.value) << ") "
                     <<   locMLString()
                     << ")";

    case T_LVAL:
      return stringc << "(" << exp_Lval << " "
                     <<   lval->toMLString()
                     << ")";

    case T_UNOP:
      return stringc << "(" << exp_UnOp << " "
                     <<   unOpMLText(unop.op) << " "
                     <<   unop.exp->toMLString() << " "
                     <<   locMLString()
                     << ")";

    case T_BINOP:
      return stringc << "(" << exp_BinOp << " "
                     <<   binOpMLText(binop.op) << " "
                     <<   binop.left->toMLString() << " "
                     <<   binop.right->toMLString() << " "
                     <<   locMLString()
                     << ")";

    case T_CASTE:
      return stringc << "(" << exp_CastE << " "
                     <<   caste.type->toMLValue() << " "
                     <<   caste.exp->toMLString() << " "
                     <<   locMLString()
                     << ")";

    case T_ADDROF:
      return stringc << "(" << exp_AddrOf << " "
                     <<   addrof.lval->toMLString() << " "
                     <<   locMLString()
                     << ")";
  }
}


CilExpr *newIntLit(CilExtraInfo tn, int val)
{
  CilExpr *ret = new CilExpr(tn, CilExpr::T_LITERAL);
  ret->lit.value = val;
  return ret;
}

CilExpr *newUnaryExpr(CilExtraInfo tn, UnaryOp op, CilExpr *expr)
{
  CilExpr *ret = new CilExpr(tn, CilExpr::T_UNOP);
  ret->unop.op = op;
  ret->unop.exp = expr;
  return ret;
}

CilExpr *newBinExpr(CilExtraInfo tn, BinOp op, CilExpr *e1, CilExpr *e2)
{
  CilExpr *ret = new CilExpr(tn, CilExpr::T_BINOP);
  ret->binop.op = op;
  ret->binop.left = e1;
  ret->binop.right = e2;
  return ret;
}

CilExpr *newCastExpr(CilExtraInfo tn, Type const *type, CilExpr *expr)
{
  CilExpr *ret = new CilExpr(tn, CilExpr::T_CASTE);
  ret->caste.type = type;
  ret->caste.exp = expr;
  return ret;
}

CilExpr *newAddrOfExpr(CilExtraInfo tn, CilLval *lval)
{
  CilExpr *ret = new CilExpr(tn, CilExpr::T_ADDROF);
  ret->addrof.lval = lval;
  return ret;
}


// ------------------- CilLval -------------------
CilLval::CilLval(CilExtraInfo tn, LTag tag)
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
      return newVarRef(extra(), varref.var);

    case T_DEREF:
      return newDeref(extra(), deref.addr->clone());

    case T_FIELDREF:
      return newFieldRef(extra(), 
                         fieldref.record->clone(), 
                         fieldref.field,
                         fieldref.recType);

    case T_CASTL:
      return newCastLval(extra(),
                         castl.type,
                         castl.lval->clone());

    case T_ARRAYELT:
      return newArrayAccess(extra(),
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


#define MKTAG(n, t) MAKE_ML_TAG(lval, n, t)
MKTAG(0, Var)
MKTAG(1, Deref)
MKTAG(2, Field)
MKTAG(3, CastL)
MKTAG(4, ArrayElt)
#undef MKTAG


string CilLval::toMLString() const
{
  switch (ltag) {
    default: xfailure("bad tag");

    case T_VARREF:
      // TODO: Var        of varinfo * offset * location
      // Var        of varinfo * location
      return mlTuple2(lval_Var,
                      varref.var->toMLValue(),
                      locMLString());

    case T_DEREF:
      return stringc << "(" << lval_Deref << " "
                     <<   deref.addr->toMLString() << " "
                     <<   locMLString()
                     << ")";

    case T_FIELDREF:
      return stringc << "(" << lval_Field << " "
                     <<   fieldref.record->toMLString() << " "
                     <<   fieldref.field->toMLValue() << " "
                     <<   locMLString()
                     << ")";

    case T_CASTL:
      return stringc << "(" << lval_CastL << " "
                     <<   castl.type->toMLValue() << " "
                     <<   castl.lval->toMLString() << " "
                     <<   locMLString()
                     << ")";

    case T_ARRAYELT:
      return stringc << "(" << lval_ArrayElt << " "
                     <<   arrayelt.array->toMLString() << " "
                     <<   arrayelt.index->toMLString() << " "
                     << ")";
  }
}


CilLval *newVarRef(CilExtraInfo tn, Variable *var)
{
  // must be an lvalue
  xassert(!var->isEnumValue());

  CilLval *ret = new CilLval(tn, CilLval::T_VARREF);
  ret->varref.var = var;
  return ret;
}

CilExpr *newVarRefExpr(CilExtraInfo tn, Variable *var)
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


CilLval *newDeref(CilExtraInfo tn, CilExpr *ptr)
{
  CilLval *ret = new CilLval(tn, CilLval::T_DEREF);
  ret->deref.addr = ptr;
  return ret;
}

CilLval *newFieldRef(CilExtraInfo tn, CilLval *record, Variable *field,
                     CompoundType const *recType)
{
  CilLval *ret = new CilLval(tn, CilLval::T_FIELDREF);
  ret->fieldref.record = record;
  ret->fieldref.field = field;
  ret->fieldref.recType = recType;
  return ret;
}

CilLval *newCastLval(CilExtraInfo tn, Type const *type, CilLval *lval)
{
  CilLval *ret = new CilLval(tn, CilLval::T_CASTL);
  ret->castl.type = type;
  ret->castl.lval = lval;
  return ret;
}

CilLval *newArrayAccess(CilExtraInfo tn, CilExpr *array, CilExpr *index)
{
  CilLval *ret = new CilLval(tn, CilLval::T_ARRAYELT);
  ret->arrayelt.array = array;
  ret->arrayelt.index = index;
  return ret;
}


// ---------------------- CilInst ----------------
int CilInst::numAllocd = 0;
int CilInst::maxAllocd = 0;

CilInst::CilInst(CilExtraInfo tn, ITag tag)
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
      return newAssignInst(extra(),
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


#define MKTAG(n, t) MAKE_ML_TAG(instr, n, t)
MKTAG(0, Set)
MKTAG(1, Call)
MKTAG(2, Asm)
#undef MKTAG


void CilInst::printTree(int ind, ostream &os, bool ml) const
{
  if (itag == T_CALL) {
    call->printTree(ind, os, ml);
    return;
  }

  if (!ml) {
    indent(ind, os);
  }
  else {
    // don't indent, we're embedded
  }

  switch (itag) {
    case T_CALL:
      // handled above already; not reached

    case T_ASSIGN:
      if (!ml) {
        os << "assign " << assign.lval->toString()
           << " := " << assign.expr->toString()
           << ";" << locComment();
      }
      else {
        os << "(" << instr_Set << " "
           <<   assign.lval->toMLString() << " "
           <<   assign.expr->toMLString() << " "
           <<   locMLString()
           << ")";
      }
      break;

    case NUM_ITAGS:
      xfailure("bad tag");
  }
}


CilInst *newAssignInst(CilExtraInfo tn, CilLval *lval, CilExpr *expr)
{
  xassert(lval && expr);
  CilInst *ret = new CilInst(tn, CilInst::T_ASSIGN);
  xassert(lval->isLval());    // stab in the dark ..
  ret->assign.lval = lval;
  ret->assign.expr = expr;
  return ret;
}


// -------------------- CilFnCall -------------------
CilFnCall::CilFnCall(CilExtraInfo tn, CilLval *r, CilExpr *f)
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
    new CilFnCall(extra(), nullableCloneLval(result), func->clone());

  FOREACH_OBJLIST(CilExpr, args, iter) {
    ret->args.append(iter.data()->clone());
  }

  return ret;
}


void CilFnCall::printTree(int ind, ostream &os, bool ml) const
{
  if (!ml) {
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

  else /*ml*/ { 
    // don't indent, we're embedded
    os << "(" << instr_Call << " ";
    if (result) {
      os << result->toMLString() << " ";
    }
    else {
      os << mlNil() << " ";
    }

    os << "[";
    int ct=0;
    FOREACH_OBJLIST(CilExpr, args, iter) {
      if (++ct > 1) {
        os << "; ";
      }
      os << iter.data()->toMLString();
    }
    os << "] "
       << locMLString()
       << ")";
  }
}


void CilFnCall::appendArg(CilExpr *arg)
{
  args.append(arg);
}


CilFnCall *newFnCall(CilExtraInfo tn, CilLval *result, CilExpr *fn)
{
  return new CilFnCall(tn, result, fn);
}


// ------------------ CilStmt ---------------
int CilStmt::numAllocd = 0;
int CilStmt::maxAllocd = 0;

CilStmt::CilStmt(CilExtraInfo tn, STag tag)
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
      return newWhileLoop(extra(), 
                          loop.cond->clone(), 
                          loop.body->clone());

    case T_IFTHENELSE:
      return newIfThenElse(extra(),
                           ifthenelse.cond->clone(),
                           ifthenelse.thenBr->clone(),
                           ifthenelse.elseBr->clone());

    case T_LABEL:
      return newLabel(extra(), *(label.name));

    case T_JUMP:
      return newGoto(extra(), *(jump.dest));

    case T_RET:
      return newReturn(extra(), nullableCloneExpr(ret.expr));

    case T_SWITCH:
      return newSwitch(extra(), 
                       switchStmt.expr->clone(),
                       switchStmt.body->clone());

    case T_CASE:
      return newCase(extra(), caseStmt.value);

    case T_INST:
      return newInst(extra(), inst.inst->clone());

    case T_DEFAULT:
      return newDefault(extra());
  }

  xfailure("bad tag");
}


#define MKTAG(n, t) MAKE_ML_TAG(stmt, n, t)
MKTAG(0, Skip)
MKTAG(1, Sequence)
MKTAG(2, While)
MKTAG(3, IfThenElse)
MKTAG(4, Label)
MKTAG(5, Goto)
MKTAG(6, Return)
MKTAG(7, Switch)
MKTAG(8, Case)
MKTAG(9, Default)
MKTAG(10, Break)
MKTAG(11, Continue)
MKTAG(12, Instruction)
#undef MKTAG


void CilStmt::printTree(int ind, ostream &os, bool ml,
                        char const *mlLineEnd) const
{
  if (stag == T_COMPOUND) {
    comp->printTree(ind, os, ml);
    return;
  }
  else if (stag == T_INST) {
    if (!ml) {
      inst.inst->printTree(ind, os, ml);
    }
    else {
      indent(ind, os) << "(" << stmt_Instruction << " ";
      inst.inst->printTree(ind, os, ml);
      os << ")" << mlLineEnd;
    }
    return;
  }

  indent(ind, os);

  if (!ml) {
    switch (stag) {
      case T_COMPOUND:
      case T_INST:
        // handled above already; not reached

      case T_LOOP:
        os << "while ( " << loop.cond->toString()
           << " ) {" << locComment();
        loop.body->printTree(ind+2, os, ml);
        indent(ind, os) << "}\n";
        break;

      case T_IFTHENELSE:
        os << "if ( " << ifthenelse.cond->toString()
           << ") {" << locComment();
        ifthenelse.thenBr->printTree(ind+2, os, ml);
        indent(ind, os) << "}\n";
        indent(ind, os) << "else {\n";
        ifthenelse.elseBr->printTree(ind+2, os, ml);  
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
        switchStmt.body->printTree(ind+2, os, ml);
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

  else /*ml*/ {
    switch (stag) {
      case T_COMPOUND:
      case T_INST:
        // handled above already; not reached

      case T_LOOP:
        os << "(" << stmt_While << " "
           <<   loop.cond->toMLString() << endl;
        loop.body->printTree(ind+2, os, ml, "\n");
        indent(ind, os) << ")";
        break;

      case T_IFTHENELSE:
        os << "(" << stmt_IfThenElse << " "
           <<   ifthenelse.cond->toMLString() << endl;
        ifthenelse.thenBr->printTree(ind+2, os, ml, "\n");
        ifthenelse.elseBr->printTree(ind+2, os, ml, "\n");
        indent(ind, os) << ")";
        break;

      case T_LABEL:
        os << "(" << stmt_Label << " "
           <<  mlString(*(label.name))
           << ")";
        break;

      case T_JUMP:
        os << "(" << stmt_Goto << " "
           <<  mlString(*(jump.dest))
           << ")";
        break;

      case T_RET:
        os << "(" << stmt_Return << " ";
        if (ret.expr) {
          os << mlSome(ret.expr->toString());
        }
        else {
          os << mlNone();
        }
        os << ")";
        break;

      case T_SWITCH:
        os << "(" << stmt_Switch << " "
           <<   switchStmt.expr->toString() << endl;
        switchStmt.body->printTree(ind+2, os, ml, "\n");
        indent(ind, os) << ")";
        break;

      case T_CASE:
        os << "(" << stmt_Case << " "
           <<   mlInt(caseStmt.value)
           << ")";
        break;

      case T_DEFAULT:
        os << "(" << stmt_Default << ")";
        break;

      case NUM_STAGS:
        xfailure("bad tag");
    }

    os << mlLineEnd;
  }
}


CilStmt *newWhileLoop(CilExtraInfo tn, CilExpr *expr, CilStmt *body)
{
  xassert(expr && body);
  CilStmt *ret = new CilStmt(tn, CilStmt::T_LOOP);
  ret->loop.cond = expr;
  ret->loop.body = body;
  return ret;
}

CilStmt *newIfThenElse(CilExtraInfo tn, CilExpr *cond, CilStmt *thenBranch, CilStmt *elseBranch)
{
  xassert(cond && thenBranch && elseBranch);
  CilStmt *ret = new CilStmt(tn, CilStmt::T_IFTHENELSE);
  ret->ifthenelse.cond = cond;
  ret->ifthenelse.thenBr = thenBranch;
  ret->ifthenelse.elseBr = elseBranch;
  return ret;
}

CilStmt *newLabel(CilExtraInfo tn, LabelName label)
{
  CilStmt *ret = new CilStmt(tn, CilStmt::T_LABEL);
  ret->label.name = new LabelName(label);
  return ret;
}

CilStmt *newGoto(CilExtraInfo tn, LabelName label)
{
  CilStmt *ret = new CilStmt(tn, CilStmt::T_JUMP);
  ret->jump.dest = new LabelName(label);
  return ret;
}

CilStmt *newReturn(CilExtraInfo tn, CilExpr *expr /*nullable*/)
{
  CilStmt *ret = new CilStmt(tn, CilStmt::T_RET);
  ret->ret.expr = expr;
  return ret;
}

CilStmt *newSwitch(CilExtraInfo tn, CilExpr *expr, CilStmt *body)
{
  xassert(expr && body);
  CilStmt *ret = new CilStmt(tn, CilStmt::T_SWITCH);
  ret->switchStmt.expr = expr;
  ret->switchStmt.body = body;
  return ret;
}

CilStmt *newCase(CilExtraInfo tn, int val)
{
  CilStmt *ret = new CilStmt(tn, CilStmt::T_CASE);
  ret->caseStmt.value = val;
  return ret;
}

CilStmt *newDefault(CilExtraInfo tn)
{
  CilStmt *ret = new CilStmt(tn, CilStmt::T_DEFAULT);
  return ret;
}

CilStmt *newInst(CilExtraInfo tn, CilInst *inst)
{
  xassert(inst);
  CilStmt *ret = new CilStmt(tn, CilStmt::T_INST);
  ret->inst.inst = inst;
  return ret;
}

CilStmt *newAssign(CilExtraInfo tn, CilLval *lval, CilExpr *expr)
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
      iter.data()->printTree(ind, os, false /*ml*/);
    }
    catch (xBase &x) {
      os << "$$$ // error printing: " << x << endl;
    }
  }
}


// ----------------- CilCompound ---------------------
CilCompound::CilCompound(CilExtraInfo tn)
  : CilStmt(tn, CilStmt::T_COMPOUND)
{}

CilCompound::~CilCompound()
{
  comp = NULL;          // prevent infinite recursion
}


CilCompound *CilCompound::clone() const
{
  CilCompound *ret = new CilCompound(extra());

  FOREACH_OBJLIST(CilStmt, stmts, iter) {
    ret->append(iter.data()->clone());
  }

  return ret;
}


void CilCompound::printTree(int ind, ostream &os, bool ml, 
                            char const *mlLineEnd) const
{
  if (!ml) {
    printTreeNoBraces(ind, os);
  }
  else /*ml*/ {
    indent(ind, os) << "(" << stmt_Sequence;
    if (stmts.isEmpty()) {
      // visual optimization
      os << " [])";
    }
    else {
      os << " [\n";
      int ct=stmts.count();
      FOREACH_OBJLIST(CilStmt, stmts, iter) {
        ct--;
        iter.data()->printTree(ind+2, os, ml,
                               ct > 0? " ;\n" : "\n");
      }
      indent(ind, os) << "])";
    }

    os << mlLineEnd;
  }
}


CilCompound *newCompound(CilExtraInfo tn)
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
    iter.data()->printTree(ind, os, false /*ml*/);
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


void CilFnDefn::printTree(int ind, ostream &os, bool stmts, bool ml) const
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
    bodyStmt.printTree(ind+2, os, ml);
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


void CilProgram::printTree(int ind, ostream &os, bool ml) const
{
  SFOREACH_OBJLIST(Variable, globals, iter) {
    indent(ind, os)
       << "global " << iter.data()->name << " : ";
    if (!ml) {
      os << iter.data()->type->toString(0 /*depth*/) << " ;\n";
    }
    else {
      os << iter.data()->type->toMLValue() << " ;\n";
    }
  }

  FOREACH_OBJLIST(CilFnDefn, funcs, iter) {
    iter.data()->printTree(ind, os, true /*stmts*/, ml);
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

