// cc_tcheck.cc            see license.txt for copyright and terms of use
// C++ typechecker, implemented as methods declared in cc.ast

#include "cc.ast.gen.h"     // C++ AST
#include "cc_env.h"         // Env
#include "trace.h"          // trace


// ------------------- TranslationUnit --------------------
void TranslationUnit::tcheck(Env &env)
{
  FOREACH_ASTLIST_NC(TopForm, topForms, iter) {
    iter.data()->tcheck(env);
  }
}


// --------------------- TopForm ---------------------
void TF_decl::tcheck(Env &env)
{
  env.setLoc(loc);
  decl->tcheck(env);
}

void TF_func::tcheck(Env &env)
{
  env.setLoc(loc);
  f->tcheck(env);
}


// --------------------- Function -----------------
void Function::tcheck(Env &env)
{
  // construct the type of the function
  Type const *retTypeSpec = retspec->tcheck(env);
  nameParams->tcheck(env, retTypeSpec, 
                     (DeclFlags)(dflags | DF_DEFINITION));

  if (! nameParams->var->type->isFunctionType() ) {
    env.error("function declarator must be of function type");
    return;
  }        
  
  // the parameters will have been entered into the parameter
  // scope, but that's gone now; make a new scope for the
  // function body and enter the parameters into that
  env.enterScope();
  FunctionType const &ft = nameParams->var->type->asFunctionTypeC();
  FOREACH_OBJLIST(FunctionType::Param, ft.params, iter) {
    env.addVariable(iter.data()->decl);
  }

  // check the body in the new scope as well
  Statement *sel = body->tcheck(env);
  xassert(sel == body);     // compounds are never ambiguous
  
  // close the new scope
  env.exitScope();
}


// MemberInit

// -------------------- Declaration -------------------
void Declaration::tcheck(Env &env)
{
  Type const *specType = spec->tcheck(env);

  FAKELIST_FOREACH_NC(Declarator, decllist, iter) {
    iter->tcheck(env, specType, dflags);
  }
}


//  -------------------- ASTTypeId -------------------
void ASTTypeId::tcheck(Env &env)
{
  Type const *specType = spec->tcheck(env);
  decl->tcheck(env, specType, DF_NONE);
}


Type const *ASTTypeId::getType() const
{
  return decl->var->type;
}


// PQName

// --------------------- TypeSpecifier --------------
Type const *TS_name::tcheck(Env &env)
{
  Variable *var = env.lookupPQVariable(name);
  if (!var) {
    return env.error(stringc
      << "there is no typedef called `" << *name << "'",
      true /*disambiguates*/);
  }

  if (!var->hasFlag(DF_TYPEDEF)) {
    return env.error(stringc
      << "variable name `" << *name << "' used as if it were a type",
      true /*disambiguates*/);
  }

  Type const *ret = applyCVToType(cv, var->type);
  if (!ret) {
    return env.error(stringc
      << "cannot apply const/volatile to type `" << ret->toString() << "'");
  }
  else {
    return ret;
  }
}


Type const *TS_simple::tcheck(Env &env)
{
  return getSimpleType(id);
}


Type const *makeNewCompound(CompoundType *&ct, Env &env, StringRef name,
                            SourceLocation const &loc, TypeIntr keyword)
{
  ct = new CompoundType((CompoundType::Keyword)keyword, name);
  ct->forward = false;
  bool ok = env.addCompound(ct);
  xassert(ok);     // already checked that it was ok

  // make the implicit typedef
  Type const *ret = makeType(ct);
  Variable *tv = new Variable(loc, name, ret, DF_TYPEDEF);
  ct->typedefVar = tv;
  ok = env.addVariable(tv);
  if (!ok) {
    return env.error(stringc
      << "implicit typedef associated with " << ct->keywordAndName()
      << " conflicts with an existing typedef or variable");
  }

  return ret;
}


Type const *TS_elaborated::tcheck(Env &env)
{
  if (keyword == TI_ENUM) {
    EnumType const *et = env.lookupPQEnum(name);
    if (!et) {
      return env.error(stringc
        << "there is no enum called `" << name << "'");
    }

    return makeType(et);
  }

  else {
    CompoundType *ct = env.lookupPQCompound(name);
    if (!ct) {
      if (name->hasQualifiers()) {
        return env.error(stringc
          << "there is no " << toString(keyword) << " called `" << *name << "'");
      }
      else {
        // forward declaration (actually, cppstd sec. 3.3.1 has some
        // rather elaborate rules for deciding in which contexts this is
        // right, but for now I'll just remark that my implementation
        // has a BUG since it doesn't quite conform)
        return makeNewCompound(ct, env, name->name, loc, keyword);
      }
    }

    // check that the keywords match; these are different enums,
    // but they agree for the three relevant values
    if ((int)keyword != (int)ct->keyword) {
      return env.error(stringc
        << "you asked for a " << toString(keyword) << " called `"
        << name << "', but that's actually a " << toString(ct->keyword));
    }

    return makeType(ct);
  }
}


Type const *TS_classSpec::tcheck(Env &env)
{
  // see if the environment already has this name
  CompoundType *ct = env.lookupCompound(name, true /*innerOnly*/);
  Type const *ret;
  if (ct) {
    // check that the keywords match
    if ((int)ct->keyword != (int)keyword) {
      return env.error(stringc
        << "there is already a " << ct->keywordAndName()
        << ", but here you're defining a " << toString(keyword)
        << " " << name);
    }

    // check that the previous was a forward declaration
    if (!ct->forward) {
      return env.error(stringc
        << ct->keywordAndName() << " has already been defined");
    }

    ret = makeType(ct);
  }

  else {
    // no existing compound; make a new one
    ret = makeNewCompound(ct, env, name, loc, keyword);
  }

  // look at the base class specifications
  if (bases) {
    env.unimp("inheritance");
  }

  // open a scope, and install 'ct' as the compound which is
  // being built
  env.enterScope();
  env.scope()->curCompound = ct;
  env.scope()->curAccess = (keyword==TI_CLASS? AK_PRIVATE : AK_PUBLIC);

  // look at members
  FOREACH_ASTLIST_NC(Member, members->list, iter) {
    iter.data()->tcheck(env);
  }

  env.exitScope();

  return ret;
}
  

Type const *TS_enumSpec::tcheck(Env &env)
{
  EnumType *et = new EnumType(name);
  Type *ret = makeType(et);

  FAKELIST_FOREACH_NC(Enumerator, elts, iter) {
    iter->tcheck(env, et, ret);
  }

  env.addEnum(et);

  return ret;
}


// BaseClass
// MemberList

// ---------------------- Member ----------------------
void MR_decl::tcheck(Env &env)
{                   
  // the declaration knows to add its variables to
  // the curCompound
  d->tcheck(env);
}

void MR_func::tcheck(Env &env)
{
  env.unimp("member function");
}

void MR_access::tcheck(Env &env)
{
  env.scope()->curAccess = k;
}


// -------------------- Enumerator --------------------
void Enumerator::tcheck(Env &env, EnumType *parentEnum, Type *parentType)
{
  Variable *v = new Variable(loc, name, parentType, DF_ENUMERATOR);

  int enumValue = parentEnum->nextValue;
  if (expr) {
    // will either set 'enumValue', or print (add) an error message
    expr->constEval(env, enumValue);
  }

  parentEnum->addValue(name, enumValue, v);
  parentEnum->nextValue = enumValue + 1;
  
  // cppstd sec. 3.3.1: 
  //   "The point of declaration for an enumerator is immediately after
  //   its enumerator-definition. [Example:
  //     const int x = 12;
  //     { enum { x = x }; }
  //   Here, the enumerator x is initialized with the value of the 
  //   constant x, namely 12. ]"
  if (!env.addVariable(v)) {
    env.error(stringc
      << "enumerator " << name << " conflicts with an existing variable "
      << "or typedef by the same name");
  }
}


// -------------------- Declarator --------------------
void Declarator::tcheck(Env &env, Type const *spec, DeclFlags dflags)
{
  // get the variable from the IDeclarator
  var = decl->tcheck(env, spec, dflags);

  // cppstd, sec. 3.3.1: 
  //   "The point of declaration for a name is immediately after 
  //   its complete declarator (clause 8) and before its initializer 
  //   (if any), except as noted below."
  // (where "below" talks about enumerators, class members, and
  // class names)
  //
  // However, since the bottom of the recursion for IDeclarators
  // is always D_name, it's equivalent to add the name to the
  // environment then instead of here.

  if (init) {
    // TODO: check the initializer for compatibility with
    // the declared type
    
    // TODO: check compatibility with dflags; e.g. we can't allow
    // an initializer for a global variable declared with 'extern'

    init->tcheck(env);
  }
}


//  ------------------ IDeclarator ------------------
Variable *IDeclarator::tcheck(Env &env, Type const *spec, DeclFlags dflags)
{
  // apply the stars left to right; the leftmost is the innermost
  FAKELIST_FOREACH_NC(PtrOperator, stars, iter) {
    spec = makePtrOperType(iter->isPtr? PO_POINTER : PO_REFERENCE,
                           iter->cv, spec);
  }

  // now go inside and apply the specific type constructor
  return itcheck(env, spec, dflags);
}


Variable *D_name::itcheck(Env &env, Type const *spec, DeclFlags dflags)
{
  env.setLoc(loc);
  
  if (!name) {
    // no name, nothing to enter in environment
    return new Variable(loc, NULL, spec, dflags);
  }

  // if we're not in a class member list, and the type is not a
  // function type, and 'extern' is not specified, then this is
  // a definition
  if (!env.scope()->curCompound &&
      !spec->isFunctionType() &&
      !(dflags & DF_EXTERN)) {
    dflags = (DeclFlags)(dflags | DF_DEFINITION);
  }

  // has this variable already been declared?
  Variable *prior = NULL;

  if (name->hasQualifiers()) {
    // the name has qualifiers, which means it *must* be
    // declared somewhere
    prior = env.lookupPQVariable(name);
    if (!prior) {
      env.error(stringc
        << "undeclared identifier `" << *name << "'");

    makeDummyVar:
      // the purpose of this is to allow the caller to have a workable
      // object, so we can continue making progress diagnosing errors
      // in the program; this won't be entered in the environment, even
      // though the 'name' is not NULL
      return new Variable(loc, name->name, spec, dflags);
    }
    
    // this intends to be the definition of a class member; make sure
    // the code doesn't try to define a nonstatic data member
    if (prior->hasFlag(DF_MEMBER) &&
        !prior->type->isFunctionType() &&
        !prior->hasFlag(DF_STATIC)) {
      env.error(stringc
        << "cannot define nonstatic data member `" << *name << "'");
      goto makeDummyVar;
    }
  }
  else {
    // has this name already been declared in the innermost scope?
    prior = env.lookupVariable(name->name, true /*innerOnly*/);
  }

  // did we find something?
  if (prior) {
    // check for violation of the One Definition Rule
    if (prior->hasFlag(DF_DEFINITION) &&
        (dflags & DF_DEFINITION)) {
      env.error(stringc
        << "duplicate definition for `" << *name << "'");
      goto makeDummyVar;
    }

    // check that the types match
    if (!prior->type->equals(spec)) {
      env.error(stringc
        << "prior declaration of `" << *name << "' had type `"
        << prior->type->toString() << "', but this one uses `"
        << spec->toString() << "'");
      goto makeDummyVar;
    }

    // ok, use the prior declaration, but update the 'loc'
    // if this is the definition
    if (dflags & DF_DEFINITION) {
      trace("odr") << "def'n at " << loc.toString()
                   << " overrides decl at " << prior->loc.toString()
                   << endl;
      prior->loc = loc;
      prior->setFlag(DF_DEFINITION);
      prior->clearFlag(DF_EXTERN);
    }
    return prior;
  }

  // no prior declaration, make a new variable and put it
  // into the environment (see comments in Declarator::tcheck
  // regarding point of declaration)
  Variable *var = new Variable(loc, name->name, spec, dflags);
  if (!var->type->isError()) {
    env.addVariable(var);
  }

  // are we inside a class member list?  if so, then
  // add this to the class
  CompoundType *ct = env.scope()->curCompound;
  if (ct) {
    if (ct->getNamedField(var->name)) {
      env.error(stringc
        << "duplicate declaration of class member `" << *name << "'");
    }
    else {
      AccessKeyword access = env.scope()->curAccess;
      trace("env") << "added " << toString(access)
                   << " field `" << var->name
                   << "' of type `" << var->type->toString()
                   << "' to " << ct->keywordAndName() << endl;
      ct->addField(var->name, env.scope()->curAccess, var->type, var);
      var->setFlag(DF_MEMBER);
    }
  }

  else {
    // not inside a class member list; set the DF_DEFINITION
    // flag for variables that don't say DF_EXTERN
    if (!var->type->isFunctionType() &&
        !(dflags & DF_EXTERN)) {
      var->setFlag(DF_DEFINITION);
    }
  }

  return var;

}


Variable *D_operator::itcheck(Env &env, Type const *spec, DeclFlags dflags)
{
  env.unimp("operator declarator");
  return NULL;    // will trigger segfault if used..
}


Variable *D_func::itcheck(Env &env, Type const *retSpec, DeclFlags dflags)
{
  env.setLoc(loc);

  FunctionType *ft = new FunctionType(retSpec, cv);

  // make a new scope for the parameter list
  env.enterScope();

  // typecheck and add the parameters
  FAKELIST_FOREACH_NC(ASTTypeId, params, iter) {
    iter->tcheck(env);
    Variable *v = iter->decl->var;

    ft->addParam(new FunctionType::Param(v->name, v->type, v));
  }

  env.exitScope();

  if (exnSpec) {
    env.unimp("exception specification");
  }

  // now that we've constructed this function type, pass it as
  // the 'base' on to the next-lower declarator
  return base->tcheck(env, ft, dflags);
}


Variable *D_array::itcheck(Env &env, Type const *eltSpec, DeclFlags dflags)
{
  ArrayType *at;
  if (!size) {
    at = new ArrayType(eltSpec);
  }
  else {
    if (size->isE_intLit()) {
      at = new ArrayType(eltSpec, size->asE_intLitC()->i);
    }
    else {
      // TODO: do a true const-eval of the expression
      env.error("array size isn't (obviously) a constant");
    }
  }

  return base->tcheck(env, at, dflags);
}


Variable *D_bitfield::itcheck(Env &env, Type const *spec, DeclFlags dflags)
{
  env.unimp("bitfield");
  return NULL;
}


// PtrOperator
// ExceptionSpec
// OperatorDeclarator

// ---------------------- Statement ---------------------
// return true if the list contains no disambiguating errors
bool noDisambErrors(ObjList<ErrorMsg> const &list)
{
  FOREACH_OBJLIST(ErrorMsg, list, iter) {
    if (iter.data()->disambiguates) {
      return false;    // has at least one disambiguating error
    }
  }
  return true;         // no disambiguating errors
}


// Generic ambiguity resolution:  We check all the alternatives,
// and select the one which typechecks without errors.  If
// 'priority' is true, then the alternatives are considered to be
// listed in order of preference, such that the first one to
// successfully typecheck is immediately chosen.  Otherwise, we
// complain if the number of successful alternatives is not 1.
template <class NODE>
NODE *resolveAmbiguity(NODE *ths, Env &env, char const *nodeTypeName,
                       bool priority)
{
  // grab the existing list of error messages
  ObjList<ErrorMsg> existingErrors;
  existingErrors.concat(env.errors);

  // grab location before checking the alternatives
  SourceLocation loc = env.loc();
  string locStr = env.locStr();

  // how many alternatives?
  int numAlts = 1;
  {
    for (NODE *a = ths->ambiguity; a != NULL; a = a->ambiguity) {
      numAlts++;
    }
  }

  // make an array of lists to hold the errors generated by the
  // various alternatives
  enum { MAX_ALT = 3 };
  xassert(numAlts <= MAX_ALT);
  ObjList<ErrorMsg> altErrors[MAX_ALT];

  // check each one
  int altIndex = 0;
  int numOk = 0;
  NODE *lastOk = NULL;
  int lastOkIndex = -1;
  for (NODE *alt = ths; alt != NULL; alt = alt->ambiguity, altIndex++) {
    int beforeChange = env.getChangeCount();
    alt->mid_tcheck(env);
    altErrors[altIndex].concat(env.errors);
    if (noDisambErrors(altErrors[altIndex])) {
      numOk++;
      lastOk = alt;
      lastOkIndex = altIndex;

      if (priority) {
        // the alternatives are listed in priority order, so once an
        // alternative succeeds, stop and select it
        break;
      }
    }
    else {
      // if this NODE failed to check, then it had better not
      // have modified the environment
      xassert(beforeChange == env.getChangeCount());
    }
  }

  if (numOk == 0) {
    // none of the alternatives checked out
    trace("disamb") << locStr << ": ambiguous " << nodeTypeName
                    << ": all bad\n";

    // put all the errors in and also a note about the ambiguity
    for (int i=0; i<numAlts; i++) {
      env.errors.concat(altErrors[i]);
    }
    env.errors.append(new ErrorMsg("following messages from an ambiguity", loc));
    env.errors.concat(existingErrors);
    env.error("previous messages from an ambiguity with bad alternatives");
    return ths;
  }

  else if (numOk == 1) {
    // one alternative succeeds, which is what we want
    trace("disamb") << locStr << ": ambiguous " << nodeTypeName
                    << ": selected " << lastOk->kindName() << endl;

    // put back its errors (non-disambiguating, and warnings);
    // errors associated with other alternatives will be deleted
    // automatically
    env.errors.concat(altErrors[lastOkIndex]);

    // put back pre-existing errors
    env.errors.concat(existingErrors);

    // break the ambiguity link (if any) in 'lastOk', so if someone
    // comes along and tchecks this again we can skip the last part
    // of the ambiguity list
    const_cast<NODE*&>(lastOk->ambiguity) = NULL;
    return lastOk;
  }

  else {
    // more than one alternative succeeds, not good
    trace("disamb") << locStr << ": ambiguous " << nodeTypeName
                    << ": multiple good!\n";

    // first put back the old errors
    env.errors.concat(existingErrors);

    // now complain
    env.error("more than one ambiguous alternative succeeds");
    return ths;
  }            
  
  // what is g++ smoking?  for some reason it wants this,
  // even though it's clear it is unreachable...
  xfailure("something weird happened.. this shouldn't be reachable");
  return ths;
}

Statement *Statement::tcheck(Env &env)
{
  env.setLoc(loc);

  if (!ambiguity) {
    // easy case
    mid_tcheck(env);
    return this;
  }

  // the only ambiguity for Statements I know if is S_decl vs. S_expr,
  // and this one is always resolved in favor of S_decl if the S_decl
  // is a valid interpretation [cppstd, sec. 6.8]
  if (this->isS_decl() && ambiguity->isS_expr() && 
      ambiguity->ambiguity == NULL) {
    // S_decl is first, run resolver with priority enabled
    return resolveAmbiguity(this, env, "Statement", true /*priority*/);
  }
  else if (this->isS_expr() && ambiguity->isS_decl() &&
           ambiguity->ambiguity == NULL) {
    // swap the expr and decl
    S_expr *expr = this->asS_expr();
    S_decl *decl = ambiguity->asS_decl();
    
    const_cast<Statement*&>(expr->ambiguity) = NULL;
    const_cast<Statement*&>(decl->ambiguity) = expr;
    
    // now run it with priority
    return resolveAmbiguity(static_cast<Statement*>(decl), env, 
                            "Statement", true /*priority*/);
  }
  
  // unknown ambiguity situation
  env.error("unknown statement ambiguity");
  return this;
}


void S_skip::itcheck(Env &env)
{}


void S_label::itcheck(Env &env)
{
  // this is a prototypical instance of typechecking a
  // potentially-ambiguous subtree; we have to change the
  // pointer to whatever is returned by the tcheck call
  s = s->tcheck(env);
  
  // TODO: check that the label is not a duplicate
}


void S_case::itcheck(Env &env)
{                    
  expr = expr->tcheck(env);
  s = s->tcheck(env);      
  
  // TODO: check that the expression is of a type that makes
  // sense for a switch statement, and that this isn't a 
  // duplicate case
}


void S_default::itcheck(Env &env)
{
  s = s->tcheck(env);
  
  // TODO: check that there is only one 'default' case
}


void S_expr::itcheck(Env &env)
{
  expr = expr->tcheck(env);
}


void S_compound::itcheck(Env &env)
{
  env.enterScope();

  FOREACH_ASTLIST_NC(Statement, stmts, iter) {   
    // have to potentially change the list nodes themselves
    iter.setDataLink( iter.data()->tcheck(env) );          
  }

  env.exitScope();
}


void S_if::itcheck(Env &env)
{
  // if 'cond' declares a variable, its scope is the
  // body of the "if"
  env.enterScope();

  cond->tcheck(env);
  thenBranch = thenBranch->tcheck(env);
  elseBranch = elseBranch->tcheck(env);

  env.exitScope();
}


void S_switch::itcheck(Env &env)
{
  env.enterScope();

  cond->tcheck(env);
  branches = branches->tcheck(env);

  env.exitScope();
}


void S_while::itcheck(Env &env)
{
  env.enterScope();

  cond->tcheck(env);
  body = body->tcheck(env);

  env.exitScope();
}


void S_doWhile::itcheck(Env &env)
{
  body = body->tcheck(env);
  expr = expr->tcheck(env);

  // TODO: verify that 'expr' makes sense in a boolean context
}


void S_for::itcheck(Env &env)
{
  env.enterScope();

  init = init->tcheck(env);
  cond->tcheck(env);
  after = after->tcheck(env);
  body = body->tcheck(env);

  env.exitScope();
}


void S_break::itcheck(Env &env)
{
  // TODO: verify we're in the context of a 'switch'
}


void S_continue::itcheck(Env &env)
{
  // TODO: verify we're in the context of a 'switch'
}


void S_return::itcheck(Env &env)
{
  if (expr) {
    expr = expr->tcheck(env);
    
    // TODO: verify that 'expr' is compatible with the current
    // function's declared return type
  }
  
  else {
    // TODO: check that the function is declared to return 'void'
  }
}


void S_goto::itcheck(Env &env)
{
  // TODO: verify the target is an existing label
}


void S_decl::itcheck(Env &env)
{
  decl->tcheck(env);
}


void S_try::itcheck(Env &env)
{
  body->tcheck(env);
  
  FAKELIST_FOREACH_NC(Handler, handlers, iter) {
    iter->tcheck(env);
  }
  
  // TODO: verify the handlers make sense in sequence:
  //   - nothing follows a "..." specifier
  //   - no duplicates
  //   - a supertype shouldn't be caught after a subtype
}


// ------------------- Condition --------------------
void CN_expr::tcheck(Env &env)
{
  expr = expr->tcheck(env);

  // TODO: verify 'expr' makes sense in a boolean or switch context
}


void CN_decl::tcheck(Env &env)
{
  typeId->tcheck(env);
  
  // TODO: verify the type of the variable declared makes sense
  // in a boolean or switch context
}


// ------------------- Handler ----------------------
void HR_type::tcheck(Env &env)
{           
  env.enterScope();
  
  typeId->tcheck(env);
  body->tcheck(env);

  env.exitScope();
}


void HR_default::tcheck(Env &env)
{
  body->tcheck(env);
}


// ------------------- Expression tcheck -----------------------
Expression *Expression::tcheck(Env &env)
{
  if (!ambiguity) {
    mid_tcheck(env);
    return this;
  }
  
  return resolveAmbiguity(this, env, "Expression", false /*priority*/);
}


void Expression::mid_tcheck(Env &env)
{                              
  if (type) {
    // this expression has already been checked
    return;
  }

  // check it, and store the result
  Type const *t = itcheck(env);
  type = t;
}


Type const *E_boolLit::itcheck(Env &env)
{
  return getSimpleType(ST_BOOL);
}

Type const *E_intLit::itcheck(Env &env)
{
  // TODO: what about unsigned and/or long literals?
  return getSimpleType(ST_INT);
}

Type const *E_floatLit::itcheck(Env &env)
{                                
  // TODO: doubles
  return getSimpleType(ST_FLOAT);
}

Type const *E_stringLit::itcheck(Env &env)
{                                                                     
  // TODO: should be char const *, not char *
  return new PointerType(PO_POINTER, CV_NONE, getSimpleType(ST_CHAR));
}

Type const *E_charLit::itcheck(Env &env)
{                               
  // TODO: unsigned
  return getSimpleType(ST_CHAR);
}


Type const *E_variable::itcheck(Env &env)
{
  var = env.lookupPQVariable(name);
  if (!var) {
    return env.error(stringc
      << "there is no variable called `" << *name << "'",
      true /*disambiguates*/);
  }

  if (var->hasFlag(DF_TYPEDEF)) {
    return env.error(stringc
      << "`" << *name << "' used as a variable, but it's actually a typedef",
      true /*disambiguates*/);
  }

  if (var->type->isFunctionType()) {
    // no lvalue for functions
    return var->type;
  }
  else {
    // return a reference because this is an lvalue
    return makeRefType(var->type);
  }
}


FakeList<Expression> *tcheckFakeExprList(FakeList<Expression> *list, Env &env)
{
  if (!list) {
    return list;
  }

  // check first expression
  FakeList<Expression> *ret
    = FakeList<Expression>::makeList(list->first()->tcheck(env));

  // check subsequent expressions, using a pointer that always
  // points to the node just before the one we're checking
  Expression *prev = ret->first();
  while (prev->next) {
    const_cast<Expression*&>(prev->next) = prev->next->tcheck(env);

    prev = prev->next;
  }

  return ret;
}

Type const *E_funCall::itcheck(Env &env)
{
  func = func->tcheck(env);
  args = tcheckFakeExprList(args, env);

  if (!func->type->isFunctionType()) {
    return env.error(stringc
      << "you can't use an expression of type `" << func->type->toString()
      << "' as a function");
  }

  // TODO: take into account possibility of operator overloading
  
  // TODO: I currently translate array deref into ptr arith plus
  // ptr deref; that makes it impossible to overload [] !

  // TODO: make sure the argument types are compatible
  // with the function parameters

  // type of the expr is type of the return value
  return func->type->asFunctionTypeC().retType;
}


Type const *E_constructor::itcheck(Env &env)
{
  Type const *t = type->tcheck(env);
  args = tcheckFakeExprList(args, env);

  // TODO: make sure the argument types are compatible
  // with the constructor parameters

  // TODO: prefer the "declaration" interpretation
  // when that is possible

  return t;
}


Type const *E_fieldAcc::itcheck(Env &env)
{
  obj = obj->tcheck(env);
  
  // get the type of 'obj', and make sure it's a compound
  Type const *rt = obj->type->asRval();
  CompoundType const *ct = rt->ifCompoundType();
  if (!ct) {
    return env.error(stringc
      << "non-compound `" << rt->toString()
      << "' doesn't have fields to access");
  }

  if (fieldName->hasQualifiers()) {
    // this also catches destructor invocations
    return env.unimp("fields with qualified names");
  }

  // look for the named field
  CompoundType::Field const *field = ct->getNamedField(fieldName->name);
  if (!field) {
    return env.error(stringc
      << "there is no field called `" << fieldName->name
      << "' in " << obj->type->toString());
  }

  // type of expression is type of field; possibly as an lval
  if (obj->type->isLval()) {
    return makeRefType(field->type);
  }
  else {
    return field->type;
  }
}


Type const *E_sizeof::itcheck(Env &env)
{
  expr = expr->tcheck(env);

  // TODO: is this right?
  return getSimpleType(ST_UNSIGNED_INT);
}


Type const *E_unary::itcheck(Env &env)
{
  expr = expr->tcheck(env);

  // TODO: make sure 'expr' is compatible with given operator
  // TODO: consider the possibility of operator overloading
  return getSimpleType(ST_INT);
}


Type const *E_effect::itcheck(Env &env)
{
  expr = expr->tcheck(env);

  // TODO: make sure 'expr' is compatible with given operator
  // TODO: make sure that 'expr' is an lvalue (reference type)
  // TODO: consider possibility of operator overloading
  return expr->type;
}


Type const *E_binary::itcheck(Env &env)
{
  e1 = e1->tcheck(env);
  
  // if the LHS is an array, coerce it to a pointer
  Type const *lhsType = e1->type->asRval();
  if (lhsType->isArrayType()) {
    lhsType = makePtrType(lhsType->asArrayTypeC().eltType);
  }

  e2 = e2->tcheck(env);

  // TODO: make sure 'expr' is compatible with given operator
  // TODO: consider the possibility of operator overloading
  return lhsType;      // works for pointer arith..
}


Type const *E_addrOf::itcheck(Env &env)
{
  expr = expr->tcheck(env);
  
  if (!expr->type->isLval()) {
    return env.error(stringc
      << "cannot take address of non-lvalue `" 
      << expr->type->toString() << "'");
  }
  PointerType const &pt = expr->type->asPointerTypeC();
  xassert(pt.op == PO_REFERENCE);      // that's what isLval checks

  // change the "&" into a "*"
  return makePtrType(pt.atType);
}


Type const *E_deref::itcheck(Env &env)
{
  ptr = ptr->tcheck(env);

  Type const *rt = ptr->type->asRval();
  if (!rt->isPointerType()) {
    return env.error(stringc
      << "cannot derefence non-pointer `" << rt->toString() << "'");
  }
  PointerType const &pt = rt->asPointerTypeC();
  xassert(pt.op == PO_POINTER);   // otherwise not rval!

  // dereferencing yields an lvalue
  return makeRefType(pt.atType);
}


Type const *E_cast::itcheck(Env &env)
{
  ctype->tcheck(env);
  expr = expr->tcheck(env);
  
  // TODO: check that the cast makes sense
  
  return ctype->getType();
}


Type const *E_cond::itcheck(Env &env)
{
  cond = cond->tcheck(env);
  th = th->tcheck(env);
  el = el->tcheck(env);
  
  // TODO: verify 'cond' makes sense in a boolean context
  // TODO: verify 'th' and 'el' return the same type
  
  return th->type;
}


Type const *E_comma::itcheck(Env &env)
{
  e1 = e1->tcheck(env);
  e2 = e2->tcheck(env);
  
  return e2->type;
}


Type const *E_sizeofType::itcheck(Env &env)
{
  atype->tcheck(env);

  return getSimpleType(ST_UNSIGNED_INT);
}


Type const *E_assign::itcheck(Env &env)
{
  target = target->tcheck(env);
  src = src->tcheck(env);
  
  // TODO: make sure 'target' and 'src' make sense together with 'op'
  // TODO: take operator overloading into consideration
  
  return target->type;
}


Type const *E_new::itcheck(Env &env)
{
  placementArgs = tcheckFakeExprList(placementArgs, env);

  // TODO: find an operator 'new' which accepts the set
  // of placement args
  
  atype->tcheck(env);
  Type const *t = atype->getType();

  if (ctorArgs) {
    ctorArgs->list = tcheckFakeExprList(ctorArgs->list, env);
  }

  // TODO: find a constructor in t which accepts these args
  
  return makePtrType(t);
}


Type const *E_delete::itcheck(Env &env)
{
  expr = expr->tcheck(env);

  Type const *t = expr->type->asRval();
  if (!t->isPointer()) {
    env.error(stringc
      << "can only delete pointers, not `" << t->toString() << "'");
  }
  
  return getSimpleType(ST_VOID);
}


Type const *E_throw::itcheck(Env &env)
{
  if (expr) {
    expr = expr->tcheck(env);
  }
  else {
    // TODO: make sure that we're inside a 'catch' clause
  }
  return getSimpleType(ST_VOID);
}


Type const *E_keywordCast::itcheck(Env &env)
{
  type->tcheck(env);
  expr = expr->tcheck(env);
  
  // TODO: make sure that 'expr' can be cast to 'type'
  // using the 'key'-style cast
  
  return type->getType();
}


Type const *E_typeidExpr::itcheck(Env &env)
{
  expr = expr->tcheck(env);
  
  return env.unimp("RTTI typeid of an expr");
}


Type const *E_typeidType::itcheck(Env &env)
{
  type->tcheck(env);

  return env.unimp("RTTI typeid of a type");
}


// --------------------- Expression constEval ------------------
bool Expression::constEval(Env &env, int &result) const
{
  xassert(!ambiguity);
  
  if (isE_intLit()) {
    result = asE_intLitC()->i;
    return true;
  }
  else {
    env.error(stringc << 
      "for now, " << kindName() << " is never constEval'able");
    return false;
  }
}


// ExpressionListOpt

// ----------------------- Initializer --------------------
void IN_expr::tcheck(Env &env)
{
  e = e->tcheck(env);
}


void IN_compound::tcheck(Env &env)
{
  FOREACH_ASTLIST_NC(Initializer, inits, iter) {
    iter.data()->tcheck(env);
  }
}


// InitLabel


