// tcheck.cc
// implementation of typechecker over C ast

#include "c.ast.gen.h"      // C ast
#include "cc_type.h"        // Type, AtomicType, etc.
#include "cc_env.h"         // Env
#include "strutil.h"        // quoted
#include "trace.h"          // trace

#define IN_PREDICATE(env) Restorer<bool> restorer(env.inPredicate, true)


void checkBoolean(Env &env, Type const *c, Expression const *e)
{
  if (c->isIntegerType() ||
      c->isPointerType() ||
      c->isArrayType()) {      // what the hey..
    // ok
  }
  else {
    env.err(stringc << "expression `" << e->toString() 
                    << "', type `" << c->toString()
                    << "' should be a number or pointer");
  }
}


// ------------------ TranslationUnit -----------------
void TranslationUnit::tcheck(Env &env)
{
  FOREACH_ASTLIST_NC(TopForm, topForms, iter) {
    iter.data()->tcheck(env);
  }
}


// --------------------- TopForm ---------------------
void TF_decl::tcheck(Env &env)
{
  decl->tcheck(env);
}


void TF_func::tcheck(Env &env)
{
  Type const *r = retspec->tcheck(env);
  Type const *f = nameParams->tcheck(env, r, dflags);
  xassert(f->isFunctionType());

  // write down the expected return type
  env.setCurrentRetType(r);

  // put parameters into the environment
  env.enterScope();
  FOREACH_OBJLIST(FunctionType::Param, ftype()->params, iter) {
    FunctionType::Param const *p = iter.data();
    env.addVariable(p->name, DF_NONE, p->type);
  }

  // (TODO) verify the pre/post don't have side effects, and
  // limit syntax somewhat
  {
    IN_PREDICATE(env);

    // check the precondition (again; it was checked once when the
    // declarator was checked, but we do it again to get the
    // bindings)
    FA_precondition *pre = ftype()->precondition;
    if (pre) {
      FOREACH_ASTLIST_NC(Declaration, pre->decls, iter) {
        iter.data()->tcheck(env);
      }

      checkBoolean(env, pre->expr->tcheck(env), pre->expr);
    }

    // and the postcondition
    FA_postcondition *post = ftype()->postcondition;
    if (post) {
      // the postcondition has 'result' available, as the type
      // of the return value
      env.enterScope();
      if (! ftype()->retType->isVoid()) {
        env.addVariable(env.strTable.add("result"), DF_NONE, r);
      }

      checkBoolean(env, post->expr->tcheck(env), post->expr);

      env.leaveScope();
    }
  }

  // check the body in the new environment
  body->tcheck(env);

  // clean up
  env.leaveScope();
}


// ------------------- Declaration ----------------------
void Declaration::tcheck(Env &env)
{
  // check the declflags for sanity
  // (for now, support only a very limited set)
  if (dflags == DF_NONE ||
      dflags == DF_TYPEDEF ||
      dflags == DF_STATIC ||
      dflags == DF_EXTERN) {
    // ok
  }
  else {
    env.err(stringc << "unsupported dflags: " << toString(dflags));
    dflags = DF_NONE;
  }

  // compute the base type
  Type const *base = spec->tcheck(env);

  // apply this type to each of the declared entities
  FOREACH_ASTLIST_NC(Declarator, decllist, iter) {
    iter.data()->tcheck(env, base, dflags);
  }
}


// --------------------- ASTTypeId ---------------------
Type const *ASTTypeId::tcheck(Env &env)
{
  Type const *s = spec->tcheck(env);
  return decl->tcheck(env, s, DF_NONE);
}


// ------------------- TypeSpecifier -------------------
Type const *TypeSpecifier::applyCV(Env &env, Type const *base)
{
  Type const *ret = env.applyCVToType(cv, base);
  env.errIf(!ret, stringc << "can't apply " << toString(cv)
                          << " to " << base->toString());

  return ret;
}


Type const *TS_name::tcheck(Env &env)
{
  Type const *base = env.getTypedef(name);
  env.errIf(!base, stringc << "unknown typedef " << name);

  return applyCV(env, base);
}


Type const *TS_simple::tcheck(Env &env)
{
  return applyCV(env, &CVAtomicType::fixed[id]);
}


Type const *TS_elaborated::tcheck(Env &env)
{
  AtomicType *ret;
  if (keyword != TI_ENUM) {
    ret = env.getOrAddCompound(name, (CompoundType::Keyword)keyword);
    env.errIf(!ret, stringc << name << " already declared differently");
  }
  else {
    ret = env.getOrAddEnum(name);
  }

  return env.makeCVType(ret, cv);
}


Type const *TS_classSpec::tcheck(Env &env)
{
  CompoundType *ct = env.getOrAddCompound(name, (CompoundType::Keyword)keyword);
  if (name) {
    env.errIf(!ct, stringc << name << " already declared differently");
    env.errIf(ct->isComplete(), stringc << name << " already declared");
  }
  else {
    xassert(ct);
  }

  // fill in 'ct' with its fields
  env.pushStruct(ct);      // declarations will go into 'ct'
  FOREACH_ASTLIST_NC(Declaration, members, iter) {
    iter.data()->tcheck(env);
  }
  env.popStruct();

  return env.makeCVType(ct, cv);
}


class XNonConst : public xBase {
public:
  Expression const *subexpr;     // on which it fails to be const

public:
  XNonConst() : xBase("non-const") {}
  XNonConst(XNonConst const &obj) : xBase(obj), subexpr(obj.subexpr) {}
  ~XNonConst();
};

XNonConst::~XNonConst()
{}               


int constEval(Env &env, Expression *expr)
{
  try {
    return expr->constEval(env);
  }
  catch (XNonConst &x) {
    env.err(stringc << expr->toString() << " is not const because "
                    << x.subexpr->toString() << " is not const");
    return 1;     // arbitrary (but not 0, for array sizes)
  }
}


Type const *TS_enumSpec::tcheck(Env &env)
{
  if (name) {
    EnumType *et = env.getEnum(name);
    env.errIf(et, stringc << name << " already declared");
  }

  EnumType *et = env.addEnum(name);

  // fill in 'et' with enumerators
  int nextValue = 0;
  FOREACH_ASTLIST_NC(Enumerator, elts, iter) {
    Enumerator *e = iter.data();
    if (e->expr) {
      nextValue = constEval(env, e->expr);
    }

    // add the value to the type, and put this enumerator into the
    // environment so subsequent enumerators can refer to its value
    env.addEnumerator(e->name, et, nextValue);

    // if the next enumerator doesn't specify a value, use +1
    nextValue++;
  }
  
  return env.makeCVType(et, cv);
}


// -------------------- Declarator -------------------
Type const *Declarator::tcheck(Env &env, Type const *base, DeclFlags dflags)
{
  if (env.inPredicate) {
    dflags = (DeclFlags)(dflags | DF_THMPRV);
  }

  type = decl->tcheck(env, base, dflags, this);
  name = decl->getName();

  if (init) {
    // verify the initializer is well-typed, given the type
    // of the variable it initializes
    // TODO: make the variable not visible in the init?
    init->tcheck(env, type);
  }

  return type;
}


Type const *IDeclarator::tcheck(Env &env, Type const *base, DeclFlags dflags, Declarator *declarator)
{
  FOREACH_ASTLIST(PtrOperator, stars, iter) {
    // the list is left-to-right, so the first one we encounter is
    // the one to be most immediately applied to the base type:
    //   int  * const * volatile x;
    //   ^^^  ^^^^^^^ ^^^^^^^^^^
    //   base  first    second
    base = env.makePtrOperType(PO_POINTER, iter.data()->cv, base);
  }

  // call inner function
  return itcheck(env, base, dflags, declarator);
}


Type const *D_name::itcheck(Env &env, Type const *base, DeclFlags dflags, Declarator *declarator)
{
  trace("tcheck")
    << "found declarator name: " << (name? name : "(null)")
    << ", type is " << base->toCString() << endl;

  // one way this happens is in prototypes with unnamed arguments
  if (!name) {
    // skip the following
  }
  else {
    // ignoring initial values... what I want is to create an element
    // of the abstract domain and insert that here
    if (dflags & DF_TYPEDEF) {
      env.addTypedef(name, base);
    }
    else {
      Variable *v = env.addVariable(name, dflags, base);
      v->declarator = declarator;
    }
  }

  return base;
}


Type const *D_func::itcheck(Env &env, Type const *rettype, DeclFlags dflags, Declarator *declarator)
{
  FunctionType *ft = env.makeFunctionType(rettype);

  // push a scope so the argument names aren't seen as colliding
  // with names already around
  env.enterScope();

  // build the argument types
  FOREACH_ASTLIST_NC(ASTTypeId, params, iter) {
    ASTTypeId *ti = iter.data();

    // handle "..."
    if (ti->spec->kind() == TypeSpecifier::TS_SIMPLE &&
        ti->spec->asTS_simple()->id == ST_ELLIPSIS) {
      ft->acceptsVarargs = true;
      break;
    }

    // compute the type of the parameter
    Type const *paramType = ti->tcheck(env);

    // extract the name too
    StringRef /*nullable*/ paramName = ti->decl->name;

    // add it to the type description
    FunctionType::Param *param =
      new FunctionType::Param(paramName, paramType);
    ft->params.prepend(param);    // build in wrong order initially..
  }

  // correct the argument order; this preserves linear performance
  ft->params.reverse();

  // pass the annotations along via the type language
  FOREACH_ASTLIST_NC(FuncAnnotation, ann, iter) {
    ASTSWITCH(FuncAnnotation, iter.data()) {
      ASTCASE(FA_precondition, pre) {
        IN_PREDICATE(env);

        // typecheck the precondition
        FOREACH_ASTLIST_NC(Declaration, pre->decls, iter) {
          iter.data()->tcheck(env);
        }
        pre->expr->tcheck(env);

        if (ft->precondition) {
          env.err("function already has a precondition");
        }
        else {
          ft->precondition = pre;
        }
      }

      ASTNEXT(FA_postcondition, post) {
        IN_PREDICATE(env);

        // typecheck the postcondition
        post->expr->tcheck(env);

        if (ft->postcondition) {
          env.err("function already has a postcondition");
        }
        else {
          ft->postcondition = post;
        }
      }

      ASTENDCASE
    }
  }

  env.leaveScope();

  // pass the constructed function type to base's tcheck so it can
  // further build upon the type
  return base->tcheck(env, ft, dflags, declarator);
}


Type const *D_array::itcheck(Env &env, Type const *elttype, DeclFlags dflags, Declarator *declarator)
{
  ArrayType *at;
  if (size) {
    at = env.makeArrayType(elttype, constEval(env, size));
  }
  else {
    at = env.makeArrayType(elttype);
  }

  return base->tcheck(env, at, dflags, declarator);
}


Type const *D_bitfield::itcheck(Env &env, Type const *base, DeclFlags dflags, Declarator *declarator)
{
  trace("tcheck")
    << "found bitfield declarator name: "
    << (name? name : "(null)") << endl;
  xfailure("bitfields not supported yet");
  return NULL;    // silence warning
}


// ------------------- Declarator::getName ------------------------
StringRef D_name::getName() const
  { return name; }

StringRef D_func::getName() const
  { return base->getName(); }

StringRef D_array::getName() const
  { return base->getName(); }

StringRef D_bitfield::getName() const
  { return name; }


// ----------------------- Statement ---------------------
void S_skip::tcheck(Env &env)
{}

void S_label::tcheck(Env &env)
{
  s->tcheck(env);
}

void S_case::tcheck(Env &env)
{
  s->tcheck(env);
}

void S_caseRange::tcheck(Env &env)
{
  s->tcheck(env);
}

void S_default::tcheck(Env &env)
{
  s->tcheck(env);
}

void S_expr::tcheck(Env &env)
{
  expr->tcheck(env);
}

void S_compound::tcheck(Env &env)
{
  env.enterScope();
  FOREACH_ASTLIST_NC(Statement, stmts, iter) {
    iter.data()->tcheck(env);
  }                
  env.leaveScope();
}

void S_if::tcheck(Env &env)
{
  checkBoolean(env, cond->tcheck(env), cond);
  thenBranch->tcheck(env);
  elseBranch->tcheck(env);
}

void S_switch::tcheck(Env &env)
{
  branches->tcheck(env);
}

void S_while::tcheck(Env &env)
{
  checkBoolean(env, cond->tcheck(env), cond);
  body->tcheck(env);
}

void S_doWhile::tcheck(Env &env)
{
  body->tcheck(env);
  checkBoolean(env, cond->tcheck(env), cond);
}

void S_for::tcheck(Env &env)
{
  init->tcheck(env);
  checkBoolean(env, cond->tcheck(env), cond);
  after->tcheck(env);
  body->tcheck(env);
}

void S_break::tcheck(Env &env)
{}

void S_continue::tcheck(Env &env)
{}

void S_return::tcheck(Env &env)
{
  Type const *rettype = env.getCurrentRetType();
  if (expr) {
    Type const *t = expr->tcheck(env);
    env.checkCoercible(t, rettype);
  }
  else {
    env.errIf(!rettype->isVoid(), "must supply return value for non-void function");
  }
}

void S_goto::tcheck(Env &env)
{}

void S_decl::tcheck(Env &env)
{
  decl->tcheck(env);
}


void S_assert::tcheck(Env &env)
{
  IN_PREDICATE(env);

  Type const *type = expr->tcheck(env);
  checkBoolean(env, type, expr);
}

void S_assume::tcheck(Env &env)
{
  IN_PREDICATE(env);

  checkBoolean(env, expr->tcheck(env), expr);
}

void S_invariant::tcheck(Env &env)
{
  checkBoolean(env, expr->tcheck(env), expr);
}

void S_thmprv::tcheck(Env &env)
{
  IN_PREDICATE(env);
  
  s->tcheck(env);
}



// ------------------ Expression::tcheck --------------------
Type const *Expression::tcheck(Env &env)
{
  return type = itcheck(env);
}


Type const *fixed(SimpleTypeId id)
{
  return &CVAtomicType::fixed[id];
}


Type const *E_intLit::itcheck(Env &env)
{
  return fixed(ST_INT);
}


Type const *E_floatLit::itcheck(Env &env)
{
  return fixed(ST_FLOAT);
}


Type const *E_stringLit::itcheck(Env &env)
{
  return env.makePtrOperType(PO_POINTER, CV_NONE, fixed(ST_CHAR));
}


Type const *E_charLit::itcheck(Env &env)
{
  return fixed(ST_CHAR);
}


Type const *E_structLit::itcheck(Env &env)
{
  // ignore the initializer..
  return stype->tcheck(env);
}


Type const *E_variable::itcheck(Env &env)
{
  Variable *v = env.getVariable(name);
  env.errIf(!v, stringc << "undeclared variable " << name);
  
  if ((v->declFlags & DF_THMPRV) &&
      !env.inPredicate) {
    env.err(stringc << name << " cannot be referenced outside a predicate");
  }

  return v->type;
}


#if 0
Type const *E_arrayAcc::itcheck(Env &env)
{
  Type const *itype = index->tcheck(env);
  if (!itype->isIntegerType()) {
    env.err("array index type must be integer");
  }

  Type const *lhsType = arr->tcheck(env);
  if (lhsType->isArrayType()) {
    return lhsType->asArrayTypeC().eltType;
  }
  else if (lhsType->isPointerType()) {
    return lhsType->asPointerTypeC().atType;
  }
  else {
    env.err("lhs of [] must be array or pointer");
    return itype;     // whatever
  }
}    
#endif // 0


void fatal(Env &env, Expression const *expr, Type const *type, char const *msg)
{
  env.errThrow(
    stringc << "expression `" << expr->toString()
            << "', type `" << type->toString()
            << "': " << msg);
}


Type const *E_funCall::itcheck(Env &env)
{
  // TODO: if more than one argument expression has a side effect,
  // should warn about undefined order of evaluation

  Type const *maybe = func->tcheck(env);
  if (!maybe->isFunctionType()) {
    fatal(env, func, maybe, "must be function type");
  }
  FunctionType const *ftype = &( maybe->asFunctionTypeC() );

  if (env.inPredicate) {
    env.errIf(!func->isE_variable(),
      "within predicates, function applications must be simple");
  }

  ObjListIter<FunctionType::Param> param(ftype->params);

  // check argument types
  FOREACH_ASTLIST_NC(Expression, args, iter) {
    Type const *atype = iter.data()->tcheck(env);
    if (!param.isDone()) {
      env.checkCoercible(atype, param.data()->type);
      param.adv();
    }
    else if (!ftype->acceptsVarargs) {
      env.err("too many arguments");
      break;    // won't check remaining arguments..
    }
    else {
      // we can only portably pass built-in types across
      // a varargs boundary
      checkBoolean(env, atype, iter.data());
    }
  }
  if (!param.isDone()) {
    env.err("too few arguments");
  }

  return ftype->retType;
}


Type const *E_fieldAcc::itcheck(Env &env)
{
  CompoundType const *ctype =
    &( obj->tcheck(env)->asCVAtomicTypeC()
         .atomic->asCompoundTypeC() );

  CompoundType::Field const *f = ctype->getNamedField(field);
  env.errIf(!f, stringc << "no field named " << field);
  
  return f->type;
}


Type const *E_unary::itcheck(Env &env)
{
  Type const *t = expr->tcheck(env);
  if (op == UNY_SIZEOF) {
    return fixed(ST_INT);
  }

  // just about any built-in will do...
  checkBoolean(env, t, expr);

  // TODO: verify argument to ++/-- is an lvalue..
             
  if (env.inPredicate && hasSideEffect(op)) {
    env.err("cannot have side effects in predicates");
  }

  // assume these unary operators to not widen their argument
  return t;
}


Type const *E_binary::itcheck(Env &env)
{
  Type const *t1 = e1->tcheck(env);
  Type const *t2 = e2->tcheck(env);

  checkBoolean(env, t1, e1);     // pointer is acceptable here..
  checkBoolean(env, t2, e2);

  // e.g. (short,long) -> long
  return env.promoteTypes(op, t1, t2);
}


Type const *E_addrOf::itcheck(Env &env)
{
  Type const *t = expr->tcheck(env);
  
  // TODO: check that 'expr' is an lvalue

  if (expr->isE_variable()) {
    // mark the variable as having its address taken
    Variable *v = env.getVariable(expr->asE_variable()->name);
    v->declarator->addrTaken = true;
  }

  return env.makePtrOperType(PO_POINTER, CV_NONE, t);
}


Type const *E_deref::itcheck(Env &env)
{
  Type const *t = ptr->tcheck(env);
  env.errIf(!t->isPointerType() && !t->isArrayType(), 
            stringc << "can only dereference pointers, not " << t->toCString());

  if (t->isPointerType()) {
    return t->asPointerTypeC().atType;
  }
  else {
    return t->asArrayTypeC().eltType;
  }
}


Type const *E_cast::itcheck(Env &env)
{
  // let's just allow any cast at all..
  expr->tcheck(env);
  return ctype->tcheck(env);
}


Type const *E_cond::itcheck(Env &env)
{
  checkBoolean(env, cond->tcheck(env), cond);

  Type const *t = th->tcheck(env);
  Type const *e = el->tcheck(env);

  // require both alternatives be primitive too..
  checkBoolean(env, t, th);
  checkBoolean(env, e, el);

  return env.promoteTypes(BIN_PLUS, t, e);
}


Type const *E_gnuCond::itcheck(Env &env)
{
  Type const *c = cond->tcheck(env);
  Type const *e = el->tcheck(env);

  checkBoolean(env, c, cond);
  checkBoolean(env, e, el);

  return env.promoteTypes(BIN_PLUS, c, e);                   
}


Type const *E_comma::itcheck(Env &env)
{
  e1->tcheck(env);
  return e2->tcheck(env);
}


Type const *E_sizeofType::itcheck(Env &env)
{
  size = atype->tcheck(env)->reprSize();
  return fixed(ST_INT);
}


Type const *E_assign::itcheck(Env &env)
{
  Type const *stype = src->tcheck(env);
  Type const *ttype = target->tcheck(env);

  // TODO: check that 'ttype' is an lvalue

  if (env.inPredicate) {
    env.err(stringc << "cannot have side effects in predicates: " << toString());
  }

  env.checkCoercible(stype, ttype);

  return ttype;
}


Type const *E_arithAssign::itcheck(Env &env)
{
  Type const *stype = src->tcheck(env);
  Type const *ttype = target->tcheck(env);

  // TODO: check that 'ttype' is an lvalue
  // TODO: this isn't quite right.. I should first compute
  // the promotion of stype,ttype, then verify this can
  // in turn be coerced back to ttype.. (?)

  if (env.inPredicate) {
    env.err("cannot have side effects in predicates");
  }

  // they both need to be integers
  if (!stype->isIntegerType() ||
      !ttype->isIntegerType()) {
    env.err(stringc << "arguments to " << ::toString(op)
                    << "= must both be integral");
  }

  return ttype;
}


// -------------------- Expression::constEval ----------------------
int Expression::xnonconst() const
{
  XNonConst x;
  x.subexpr = this;
  throw x;
}


int E_intLit::constEval(Env &env) const
{
  return i;
}

int E_charLit::constEval(Env &env) const
{
  return c;
}

int E_unary::constEval(Env &env) const
{
  if (op == UNY_SIZEOF) {
    return expr->type->reprSize();
  }

  int v = expr->constEval(env);
  switch (op) {
    case UNY_PLUS:   return v;
    case UNY_MINUS:  return -v;
    case UNY_NOT:    return !v;
    case UNY_BITNOT: return ~v;
    default:         return xnonconst();
  }
}


int E_binary::constEval(Env &env) const
{
  int v1 = e1->constEval(env);
  int v2 = e2->constEval(env);
  switch (op) {
    case BIN_MULT:      return v1 *  v2;
    case BIN_DIV:       return v1 /  v2;
    case BIN_MOD:       return v1 %  v2;
    case BIN_PLUS:      return v1 +  v2;
    case BIN_MINUS:     return v1 -  v2;
    case BIN_LSHIFT:    return v1 << v2;
    case BIN_RSHIFT:    return v1 >> v2;
    case BIN_LESS:      return v1 <  v2;
    case BIN_GREATER:   return v1 >  v2;
    case BIN_LESSEQ:    return v1 <= v2;
    case BIN_GREATEREQ: return v1 >= v2;
    case BIN_EQUAL:     return v1 == v2;
    case BIN_NOTEQUAL:  return v1 != v2;
    case BIN_BITAND:    return v1 &  v2;
    case BIN_BITXOR:    return v1 ^  v2;
    case BIN_BITOR:     return v1 |  v2;
    case BIN_AND:       return v1 && v2;
    case BIN_OR:        return v1 || v2;
    case BIN_IMPLIES:   return (!v1) || v2;

    default:            xfailure("bad operator");
                        return 0;   // silence warning
  }
}


int E_cast::constEval(Env &env) const
{
  return expr->constEval(env);
}


int E_sizeofType::constEval(Env &env) const
{
  return size;
}


int E_floatLit::constEval(Env &env) const { return xnonconst(); }
int E_stringLit::constEval(Env &env) const { return xnonconst(); }
int E_structLit::constEval(Env &env) const { return xnonconst(); }
int E_variable::constEval(Env &env) const { return xnonconst(); }
//int E_arrayAcc::constEval(Env &env) const { return xnonconst(); }
int E_funCall::constEval(Env &env) const { return xnonconst(); }
int E_fieldAcc::constEval(Env &env) const { return xnonconst(); }
int E_addrOf::constEval(Env &env) const { return xnonconst(); }
int E_deref::constEval(Env &env) const { return xnonconst(); }
int E_cond::constEval(Env &env) const { return xnonconst(); }
int E_gnuCond::constEval(Env &env) const { return xnonconst(); }
int E_comma::constEval(Env &env) const { return xnonconst(); }
int E_assign::constEval(Env &env) const { return xnonconst(); }
int E_arithAssign::constEval(Env &env) const { return xnonconst(); }


// -------------------- Expression::toString --------------------
// TODO: these routines dramatically under-parenthesize the output..
// not sure how/if to improve the situation easily

string E_intLit::toString() const
  { return stringc << i; }
string E_floatLit::toString() const
  { return stringc << f; }
string E_stringLit::toString() const
  { return stringc << quoted(s); }
string E_charLit::toString() const
  { return stringc << "'" << c << "'"; }
string E_structLit::toString() const
  { return stringc << "(..some type..){ ... }"; }
string E_variable::toString() const
  { return stringc << name; }
//string E_arrayAcc::toString() const
//  { return stringc << arr->toString() << "[" << index->toString() << "]"; }

string E_funCall::toString() const
{
  stringBuilder sb;
  sb << func->toString() << "(";

  int count=0;
  FOREACH_ASTLIST(Expression, args, iter) {
    if (count++) {
      sb << ", ";
    }
    sb << iter.data()->toString();
  }
  sb << ")";
  return sb;
}

string E_fieldAcc::toString() const
  { return stringc << obj->toString() << "." << field; }
string E_unary::toString() const
  { return stringc << ::toString(op) << expr->toString(); }
string E_binary::toString() const
  { return stringc << e1->toString() << ::toString(op) << e2->toString(); }
string E_addrOf::toString() const
  { return stringc << "&" << expr->toString(); }
string E_deref::toString() const
  { return stringc << "*(" << ptr->toString() << ")"; }
string E_cast::toString() const
  { return stringc << "(..some type..)" << expr->toString(); }
string E_cond::toString() const
  { return stringc << cond->toString() << "?" << th->toString() << ":" << el->toString(); }
string E_gnuCond::toString() const
  { return stringc << cond->toString() << "?:" << el->toString(); }
string E_comma::toString() const
  { return stringc << e1->toString() << ", " << e2->toString(); }
string E_sizeofType::toString() const
  { return stringc << "sizeof(..some type..)"; }
string E_assign::toString() const
  { return stringc << target->toString() << " = " << src->toString(); }
string E_arithAssign::toString() const
  { return stringc << target->toString() << " " << ::toString(op) << "= " << src->toString(); }


/*
  E_intLit
  E_floatLit
  E_stringLit
  E_charLit
  E_structLit
  E_variable
  E_arrayAcc
  E_funCall
  E_fieldAcc
  E_unary
  E_binary
  E_addrOf
  E_deref
  E_cast
  E_cond
  E_gnuCond
  E_comma
  E_sizeofType
  E_assign
  E_arithAssign
*/


// ------------------------- Initializer ---------------------------
void IN_expr::tcheck(Env &env, Type const *type)
{
  Type const *t = e->tcheck(env);
  env.checkCoercible(t, type);
}

void IN_compound::tcheck(Env &env, Type const *type)
{
  // for now, ignore labels
  
  if (type->isArrayType()) {
    ArrayType const &at = type->asArrayTypeC();

    // every element should correspond to the element type
    FOREACH_ASTLIST_NC(Initializer, inits, iter) {
      iter.data()->tcheck(env, at.eltType);
    }

    // check size restriction
    if (at.hasSize && inits.count() > at.size) {
      env.err(stringc << "initializer has " << inits.count()
                      << " elements but array only has " << at.size
                      << " elements");
    }
  }

  else if (type->isCVAtomicType() &&
           type->asCVAtomicTypeC().atomic->isCompoundType()) {
    CompoundType const &ct = type->asCVAtomicTypeC().atomic->asCompoundTypeC();
    
    if (ct.keyword == CompoundType::K_UNION) {
      env.err("I don't know how to initialize unions");
      return;
    }        

    // iterate simultanously over fields and initializers, establishing
    // correspondence
    int field = 0;
    FOREACH_ASTLIST_NC(Initializer, inits, iter) {
      if (field >= ct.numFields()) {
        env.err(stringc
          << "too many initializers; " << ct.keywordAndName()
          << " only has " << ct.numFields() << " fields, but "
          << inits.count() << " initializers are present");
        return;
      }

      CompoundType::Field const *f = ct.getNthField(field);

      // check this initializer against the field it initializes
      iter.data()->tcheck(env, f->type);
      
      field++;
    }

    // in C, it's ok to leave out initializers, since all data can
    // be initialized to 0; so we don't complain if field is still
    // less than ct.numFields()
  }

  else {
    env.err(stringc << "you can't use a compound initializer to initialize "
                    << type->toString());
  }
}
