// tcheck.cc
// implementation of typechecker over C ast

#include "c.ast.gen.h"      // C ast
#include "cc_type.h"        // Type, AtomicType, etc.
#include "cc_env.h"         // Env
#include "strutil.h"        // quoted
#include "trace.h"          // trace
#include "paths.h"          // printPaths

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
void TopForm::tcheck(Env &env)
{
  env.pushLocation(&loc);
  itcheck(env);
  env.popLocation();
}


void TF_decl::itcheck(Env &env)
{
  decl->tcheck(env);
}


void TF_func::itcheck(Env &env)
{
  // we're the current function
  env.setCurrentFunction(this);

  Type const *r = retspec->tcheck(env);
  Type const *f = nameParams->tcheck(env, r, dflags);
  xassert(f->isFunctionType());

  // put parameters into the environment
  env.enterScope();
  {
    // dig inside the 'nameParams' to find the parameters
    // (the downcast will succeed because isFunctionType succeeded, above)
    D_func *fdecl = nameParams->decl->asD_func();

    FOREACH_ASTLIST(ASTTypeId, fdecl->params, iter) {
      Variable *var = iter.data()->decl->var;
      if (var->name) {
        env.addVariable(var->name, var);
      }
      params.prepend(var);
    }
    params.reverse();
  }

  // (TODO) verify the pre/post don't have side effects, and
  // limit syntax somewhat
  {
    IN_PREDICATE(env);

    // make bindings for precondition logic variables (they would
    // have been added to the environment when typechecked above, 
    // but then we closed that scope (why? not sure))
    FA_precondition *pre = ftype()->precondition;
    if (pre) {
      FOREACH_ASTLIST_NC(Declaration, pre->decls, iter) {
        FOREACH_ASTLIST_NC(Declarator, iter.data()->decllist, iter2) {
          Variable *var = iter2.data()->var;
          env.addVariable(var->name, var);
        }
      }

      //checkBoolean(env, pre->expr->tcheck(env), pre->expr);
    }
                            
    #if 0     // typechecking postconditions happens in D_func::tcheck
    // and the postcondition
    FA_postcondition *post = ftype()->postcondition;
    if (post) {
      env.enterScope();

      // example of egcs' failure to disambiguate C++ ctor/prototype:
      //Variable result(SourceLocation(), env.strTable.add("result"),
      //                r, DF_LOGIC);

      // the postcondition has 'result' available, as the type
      // of the return value
      Variable *result = nameParams->decl->asD_func()->result;
      if (! ftype()->retType->isVoid()) {
        env.addVariable(result->name, result);
      }

      checkBoolean(env, post->expr->tcheck(env), post->expr);

      env.leaveScope();
    }
    #endif // 0
  }

  // may as well check that things are as I expect *before* I muck with them
  env.verifyFunctionEnd();

  // check the body in the new environment
  body->tcheck(env);

  // when the local are added, they are prepended
  locals.reverse();

  // clean up
  env.resolveGotos();
  env.resolveNexts(NULL /*target*/, false /*isContinue*/);
  env.leaveScope();

  // let the environment verify everything got cleaned up properly
  env.verifyFunctionEnd();

  // instrument AST with path information
  countPaths(env, this);

  if (tracingSys("printPaths")) {
    printPaths(this);
  }

  // ensure segfault if some later access occurs
  env.setCurrentFunction(NULL);
}


template <class T, class Y>      // Y is type of thing printed
void printSObjList(ostream &os, int indent, char const *label,
                   SObjList<T> const &list, Y (*map)(T const *t))
{
  ind(os, indent) << label << ":";
  SFOREACH_OBJLIST(T, list, iter) {
    os << " " << map(iter.data());
  }
  os << "\n";
}

StringRef varName(Variable const *v)
  { return v->name; }
string stmtLoc(Statement const *s)
  { return stringc << s->loc.line << ":" << s->loc.col; }

void TF_func::printExtras(ostream &os, int indent) const
{
  printSObjList(os, indent, "params", params, varName);
  printSObjList(os, indent, "locals", locals, varName);
  printSObjList(os, indent, "roots", roots, stmtLoc);
}


// ------------------- Declaration ----------------------
void Declaration::tcheck(Env &env)
{
  // check the declflags for sanity
  // (for now, support only a very limited set)
  {
    DeclFlags df = (DeclFlags)(dflags & DF_SOURCEFLAGS);
    if (df == DF_NONE ||
        df == DF_TYPEDEF ||
        df == DF_STATIC ||
        df == DF_EXTERN) {
      // ok
    }
    else {
      env.err(stringc << "unsupported dflags: " << toString(df));
      dflags = (DeclFlags)(dflags & ~DF_SOURCEFLAGS);
    }
  }

  // distinguish declarations of logic variables
  if (env.inPredicate) {
    dflags = (DeclFlags)(dflags | DF_LOGIC);
  }

  // compute the base type
  Type const *base = spec->tcheck(env);

  // apply this type to each of the declared entities
  FOREACH_ASTLIST_NC(Declarator, decllist, iter) {
    Declarator *d = iter.data();
    d->tcheck(env, base, dflags);
    
    // we just declared a local variable, if we're in a function
    TF_func *func = env.getCurrentFunction();
    if (func) {
      func->locals.prepend(d->var);
    }
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
    
    // make a record of the name introduction
    Variable *var = new Variable(e->loc, name,
                                 new CVAtomicType(et, CV_NONE), DF_NONE);
    xassert(e->var == NULL);
    e->var = var;

    // add the value to the type, and put this enumerator into the
    // environment so subsequent enumerators can refer to its value
    env.addEnumerator(e->name, et, nextValue, var);

    // if the next enumerator doesn't specify a value, use +1
    nextValue++;
  }
  
  return env.makeCVType(et, cv);
}


// -------------------- Declarator -------------------
Type const *Declarator::tcheck(Env &env, Type const *base, DeclFlags dflags)
{
  // check the internal structure, eventually also setting 'var'
  xassert(var == NULL);
  decl->tcheck(env, base, dflags, this);
  xassert(var != NULL);

  if (init) {
    // verify the initializer is well-typed, given the type
    // of the variable it initializes
    // TODO: make the variable not visible in the init?
    init->tcheck(env, var->type);
  }

  return var->type;
}


void /*Type const * */IDeclarator::tcheck(Env &env, Type const *base,
                                          DeclFlags dflags, Declarator *declarator)
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
  /*return*/ itcheck(env, base, dflags, declarator);
}


void /*Type const * */D_name::itcheck(Env &env, Type const *base,
                                      DeclFlags dflags, Declarator *declarator)
{
  trace("tcheck")
    << "found declarator name: " << (name? name : "(null)")
    << ", type is " << base->toCString() << endl;

  // construct a Variable: this is a binding introduction
  Variable *var = new Variable(loc, name, base, dflags);

  // annotate the declarator
  xassert(declarator->var == NULL);     // otherwise leak
  declarator->var = var;

  // one way this happens is in prototypes with unnamed arguments
  if (!name) {
    // skip the following
  }
  else {
    if (dflags & DF_TYPEDEF) {
      env.addTypedef(name, var);
    }
    else {
      env.addVariable(name, var);
    }
  }

  //return base;
}


void /*Type const * */D_func::itcheck(Env &env, Type const *rettype,
                                      DeclFlags dflags, Declarator *declarator)
{
  // make the result variable
  result = new Variable(loc, env.str("result"), rettype, DF_LOGIC);

  FunctionType *ft = env.makeFunctionType(rettype);
  ft->result = result;

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
    StringRef /*nullable*/ paramName = ti->decl->var->name;

    // add it to the type description
    FunctionType::Param *param =
      new FunctionType::Param(paramName, paramType, ti->decl->var);
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
        checkBoolean(env, pre->expr->tcheck(env), pre->expr);

        if (ft->precondition) {
          env.err("function already has a precondition");
        }
        else {
          ft->precondition = pre;
        }
      }

      ASTNEXT(FA_postcondition, post) {
        IN_PREDICATE(env);

        // add the result variable to the environment, so the
        // postcondition can refer to it
        env.addVariable(result->name, result);

        // typecheck the postcondition
        checkBoolean(env, post->expr->tcheck(env), post->expr);

        if (ft->postcondition) {
          env.err("function already has a postcondition");
        }
        else {
          ft->postcondition = post;
        }
      }

      ASTENDCASED
    }
  }

  env.leaveScope();

  // pass the constructed function type to base's tcheck so it can
  // further build upon the type
  /*return*/ base->tcheck(env, ft, dflags, declarator);
}


void /*Type const * */D_array::itcheck(Env &env, Type const *elttype,  
                                       DeclFlags dflags, Declarator *declarator)
{
  ArrayType *at;
  if (size) {
    at = env.makeArrayType(elttype, constEval(env, size));
  }
  else {
    at = env.makeArrayType(elttype);
  }

  /*return*/ base->tcheck(env, at, dflags, declarator);
}


void /*Type const * */D_bitfield::itcheck(Env &env, Type const *base, 
                                          DeclFlags dflags, Declarator *declarator)
{
  trace("tcheck")
    << "found bitfield declarator name: "
    << (name? name : "(null)") << endl;
  xfailure("bitfields not supported yet");
  //return NULL;    // silence warning
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
void Statement::tcheck(Env &env)
{
  env.pushLocation(&loc);

  // the default actions here are suitable for most kinds of
  // statements, but there are exceptions which require special
  // treatment, and that is elaborated below

  // any pending 'next' pointers go here
  env.resolveNexts(this /*target*/, false /*continue*/);

  // my 'next' will go to whoever's (outer) tcheck is called next
  env.addPendingNext(this /*source*/);

  // do inner typecheck
  itcheck(env);
  
  env.popLocation();
}


void S_skip::itcheck(Env &env)
{}

void S_label::itcheck(Env &env)
{
  env.addLabel(name, this);
  s->tcheck(env);
}


void connectEnclosingSwitch(Env &env, Statement *stmt, char const *kind)
{
  S_switch *sw = env.getCurrentSwitch();
  if (!sw) {
    env.err(stringc << kind << " can only appear in the context of a 'switch'");
  }
  else {
    sw->cases.append(stmt);
  }
}

void S_case::itcheck(Env &env)
{
  connectEnclosingSwitch(env, this, "'case'");
  s->tcheck(env);
}

void S_caseRange::itcheck(Env &env)
{
  connectEnclosingSwitch(env, this, "'case'");
  s->tcheck(env);
}

void S_default::itcheck(Env &env)
{
  connectEnclosingSwitch(env, this, "'default'");
  s->tcheck(env);
}


void S_expr::itcheck(Env &env)
{
  expr->tcheck(env);
}

void S_compound::itcheck(Env &env)
{
  env.enterScope();
  FOREACH_ASTLIST_NC(Statement, stmts, iter) {
    iter.data()->tcheck(env);
  }
  env.leaveScope();
}

void S_if::itcheck(Env &env)
{
  checkBoolean(env, cond->tcheck(env), cond);
  thenBranch->tcheck(env);

  // any pending 'next's should not be resolved as pointing into
  // the 'else' clause, but instead as pointing at whatever follows
  // the entire 'if' statement
  env.pushNexts();

  elseBranch->tcheck(env);

  // merge current pending nexts with those saved above
  env.popNexts();
}

void S_switch::itcheck(Env &env)
{
  // existing 'break's must be postponed
  env.pushBreaks();

  // any occurrances of 'case' will be relative to this switch
  env.pushSwitch(this);
  branches->tcheck(env);
  env.popSwitch();

  // any previous 'break's will resolve to whatever comes next
  env.popBreaks();
}


void tcheckLoop(Env &env, Statement *loop, Expression *cond,
                     Statement *body)
{
  // existing 'break's must be postponed
  env.pushBreaks();

  checkBoolean(env, cond->tcheck(env), cond);

  // any occurrances of 'continue' will be relative to this loop
  env.pushLoop(loop);
  body->tcheck(env);
  env.popLoop();

  // the body continues back into this loop
  env.resolveNexts(loop /*target*/, true /*continue*/);

  // any previous 'break's will resolve to whatever comes next
  env.popBreaks();

  // I want the loop's 'next' to point to what comes after; right
  // now it points at the body, and this will change it
  env.addPendingNext(loop /*source*/);
}

void S_while::itcheck(Env &env)
{
  tcheckLoop(env, this, cond, body);
}

void S_doWhile::itcheck(Env &env)
{
  tcheckLoop(env, this, cond, body);
}

void S_for::itcheck(Env &env)
{
  init->tcheck(env);
  after->tcheck(env);

  tcheckLoop(env, this, cond, body);
}


void S_break::itcheck(Env &env)
{
  // add myself to the list of active breaks
  env.addBreak(this);
}

void S_continue::itcheck(Env &env)
{
  Statement *loop = env.getCurrentLoop();
  if (!loop) {
    env.err("'continue' can only occur in the scope of a loop");
  }
  else {
    // take myself off the list of pending nexts
    env.clearNexts();

    // point my next at the loop
    next = makeNextPtr(loop, true /*continue*/);
  }
}

void S_return::itcheck(Env &env)
{
  // ensure my 'next' is null
  env.clearNexts();          
  xassert(next == NULL);

  Type const *rettype = env.getCurrentRetType();
  if (expr) {
    Type const *t = expr->tcheck(env);
    env.checkCoercible(t, rettype);
  }
  else {
    env.errIf(!rettype->isVoid(), "must supply return value for non-void function");
  }
}


void S_goto::itcheck(Env &env)
{
  env.addPendingGoto(target, this);
}


void S_decl::itcheck(Env &env)
{
  decl->tcheck(env);
}


void S_assert::itcheck(Env &env)
{
  IN_PREDICATE(env);

  Type const *type = expr->tcheck(env);
  checkBoolean(env, type, expr);
}

void S_assume::itcheck(Env &env)
{
  IN_PREDICATE(env);

  checkBoolean(env, expr->tcheck(env), expr);
}

void S_invariant::itcheck(Env &env)
{
  IN_PREDICATE(env);

  checkBoolean(env, expr->tcheck(env), expr);
}

void S_thmprv::itcheck(Env &env)
{
  IN_PREDICATE(env);

  s->tcheck(env);
}


// ------------------ Statement::getSuccessors ----------------
void Statement::getSuccessors(VoidList &dest, bool /*isContinue*/) const
{
  if (nextPtrStmt(next)) {
    dest.append(next);
  }
}


void S_if::getSuccessors(VoidList &dest, bool /*isContinue*/) const
{
  // the 'next' field is ignored since it always points at
  // the 'then' branch anyway

  dest.append(makeNextPtr(thenBranch, false));
  dest.append(makeNextPtr(elseBranch, false));
}


void S_switch::getSuccessors(VoidList &dest, bool /*isContinue*/) const
{
  xassert(dest.isEmpty());
  SFOREACH_OBJLIST(Statement, cases, iter) {
    dest.prepend(makeNextPtr(iter.data(), false));
  }
  dest.reverse();
}


void S_while::getSuccessors(VoidList &dest, bool isContinue) const
{
  Statement::getSuccessors(dest, isContinue);
  dest.append(makeNextPtr(body, false));
}


void S_doWhile::getSuccessors(VoidList &dest, bool isContinue) const
{
  if (isContinue) {
    // continue jumps to conditional, and can either go back
    // to the top (body) or past loop (next)
    Statement::getSuccessors(dest, isContinue);
  }

  // either way, doing the body is an option
  dest.append(makeNextPtr(body, false));
}


void S_for::getSuccessors(VoidList &dest, bool isContinue) const
{
  // though the semantics of which expressions get evaluated
  // are different depending on 'isContinue', the statement-level
  // control flow options are the same
  Statement::getSuccessors(dest, isContinue);
  dest.append(makeNextPtr(body, false));
}


string Statement::successorsToString() const
{
  VoidList succNoCont;
  getSuccessors(succNoCont, false);

  VoidList succYesCont;
  getSuccessors(succYesCont, true);

  stringBuilder sb;
  sb << "{";

  for (VoidListIter iter(succYesCont); !iter.isDone(); iter.adv()) {
    NextPtr np = iter.data();
    
    // a leading "(c)" means the successor edge is only present when
    // this node is reached via continue; a trailing "(c)" means that
    // successor is itself a continue edge; the algorithm assumes
    // that 'succYesCont' is a superset of 'succNoCont'
    SourceLocation const &loc = nextPtrStmt(np)->loc;
    sb << (succNoCont.contains(np)? " " : " (c)")
       << loc.line << ":" << loc.col
       << (nextPtrContinue(np)? "(c)" : "");
  }

  sb << " }";
  return sb;
}



// ------------------ Expression::tcheck --------------------
Type const *Expression::tcheck(Env &env)
{
  type = itcheck(env);
  countPaths(env, this);
  return type;
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
  // the initializer itself is ignored
  cout << "TODO: handle structure initializers\n";
  return stype->tcheck(env);
}


Type const *E_variable::itcheck(Env &env)
{
  Variable *v = env.getVariable(name);
  if (!v) {
    env.err(stringc << "undeclared variable: " << name);
    return fixed(ST_ERROR);
  }

  // connect this name reference to its binding introduction
  var = v;

  if ((v->flags & DF_LOGIC) &&
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

  if (hasSideEffect(op) && env.inPredicate) {
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
    v->flags = (DeclFlags)(v->flags | DF_ADDRTAKEN);
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
  e2->tcheck(env);

  return e2->type;
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


string Expression::extrasToString() const
{
  stringBuilder sb;
  sb << "paths=" << numPaths << ", type: ";
  if (type) {
    sb << type->toCString();
  }
  else {
    sb << "(null)";
  }
  return sb;
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
