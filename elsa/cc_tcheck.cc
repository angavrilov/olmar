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
  f->tcheck(env, true /*checkBody*/);
}

void TF_template::tcheck(Env &env)
{
  env.setLoc(loc);
  td->tcheck(env);
}

void TF_linkage::tcheck(Env &env)
{  
  env.setLoc(loc);
  // ignore the linkage type
  forms->tcheck(env);
}


// --------------------- Function -----------------
void Function::tcheck(Env &env, bool checkBody)
{
  if (checkBody) {
    dflags = (DeclFlags)(dflags | DF_DEFINITION);
  }

  // are we in a template function?
  bool inTemplate = env.scope()->templateParams != NULL;

  // construct the type of the function
  Type const *retTypeSpec = retspec->tcheck(env);
  DeclaratorTcheck dt(retTypeSpec, dflags);
  nameParams->tcheck(env, dt);

  if (! dt.type->isFunctionType() ) {
    env.error("function declarator must be of function type", true /*disambiguating*/);
    return;
  }

  if (!checkBody) {
    return;
  }

  bool prevDO = false;
  if (inTemplate) {
    // while checking the body, only report/use serious errors
    prevDO = env.setDisambiguateOnly(true);
  }

  // if this function was originally declared in another scope
  // (main example: it's a class member function), then start
  // by extending that scope so the function body can access
  // the class's members; that scope won't actually be modified,
  // and in fact we can check that by watching the change counter
  int prevChangeCount = 0;   // silence warning
  if (nameParams->var->scope) {
    env.extendScope(nameParams->var->scope);
    prevChangeCount = env.getChangeCount();
  }

  // the parameters will have been entered into the parameter
  // scope, but that's gone now; make a new scope for the
  // function body and enter the parameters into that
  Scope *bodyScope = env.enterScope();
  bodyScope->curFunction = this;
  FunctionType const &ft = dt.type->asFunctionTypeC();
  FOREACH_OBJLIST(Parameter, ft.params, iter) {
    Variable *v = iter.data()->decl;
    if (v->name) {
      env.addVariable(v);
    }
  }

  // is this a nonstatic member function?
  if (nameParams->var->scope &&
      nameParams->var->scope->curCompound &&
      !nameParams->var->hasFlag(DF_STATIC)) {
    CompoundType const *ct = nameParams->var->scope->curCompound;

    // make a type which is a pointer to the class that this
    // function is a member of; if the function has been declared
    // with some 'cv' flags, then those become attached to the
    // pointed-to type; the pointer itself is always 'const'
    Type const *thisType =
      makePtrOperType(PO_POINTER, CV_CONST, makeCVType(ct, ft.cv));

    // add the implicit 'this' parameter
    Variable *ths = new Variable(nameParams->var->loc, env.str("this"),
                                 thisType, DF_NONE);
    env.addVariable(ths);
  }

  // have to check the member inits after adding the parameters
  // to the environment, because the initializing expressions
  // can refer to the parameters
  if (inits) {
    tcheck_memberInits(env);
  }

  // check the body in the new scope as well
  Statement *sel = body->tcheck(env);
  xassert(sel == body);     // compounds are never ambiguous

  if (handlers) {
    tcheck_handlers(env);
  }

  // close the new scope
  env.exitScope(bodyScope);

  // stop extending the named scope, if there was one
  if (nameParams->var->scope) {
    xassert(prevChangeCount == env.getChangeCount());
    env.retractScope(nameParams->var->scope);
  }
  
  if (inTemplate) {
    env.setDisambiguateOnly(prevDO);
  }
}


CompoundType *Function::verifyIsCtor(Env &env, char const *context)
{
  // make sure this function is a class member
  CompoundType *enclosing = NULL;
  if (nameParams->var->scope) {
    enclosing = nameParams->var->scope->curCompound;
  }
  if (!enclosing) {
    env.error(stringc
      << context << " are only valid for class member "
      << "functions (constructors in particular)",
      true /*disambiguating*/);
    return NULL;
  }

  // make sure this function is a constructor; should already have
  // been mapped to the special name
  if (nameParams->var->name != env.constructorSpecialName) {
    env.error(stringc
      << context << " are only valid for constructors",
      true /*disambiguating*/);
    return NULL;
  }

  return enclosing;
}


// this is a prototype for a function down near E_funCall::itcheck
FakeList<Expression> *tcheckFakeExprList(FakeList<Expression> *list, Env &env);

void Function::tcheck_memberInits(Env &env)
{
  CompoundType *enclosing = verifyIsCtor(env, "ctor member inits");
  if (!enclosing) {
    return;
  }

  // ok, so far so good; now go through and check the member
  // inits themselves
  FAKELIST_FOREACH_NC(MemberInit, inits, iter) {
    if (iter->name->hasQualifiers()) {
      env.unimp("ctor member init with qualifiers");
      continue;
    }

    // look for the given name in the class
    Variable *v = enclosing->getNamedField(iter->name->getName(), env);
    if (v) {
      // typecheck the arguments
      iter->args = tcheckFakeExprList(iter->args, env);

      // TODO: check that the passed arguments are consistent
      // with at least one constructor of the variable's type

      // TODO: make sure that we only initialize each member once

      // TODO: provide a warning if the order in which the
      // members are initialized is different from their
      // declaration order, since the latter determines the
      // order of side effects

      continue;
    }

    // not a member name.. what about the name of base class?
    // TODO: this doesn't handle virtual base classes correctly
    bool found = false;
    FOREACH_OBJLIST(BaseClass, enclosing->bases, baseIter) {
      if (baseIter.data()->ct->name == iter->name->getName()) {
        // found the right base class to initialize
        found = true;
        
        // typecheck the arguments
        iter->args = tcheckFakeExprList(iter->args, env);

        // TODO: check that the passed arguments are consistent
        // with at least one constructor in the base class
        
        break;
      }
    }

    if (!found) {
      env.error(stringc
        << "ctor member init name `" << *(iter->name)
        << "' not found among class members or base classes",
        true /*disambiguating*/);
    }
  }
}


void Function::tcheck_handlers(Env &env)
{
  if (!verifyIsCtor(env, "ctor exception handlers")) {
    return;
  }
  
  FAKELIST_FOREACH_NC(Handler, handlers, iter) {
    iter->tcheck(env);
  }
}


// MemberInit

// -------------------- Declaration -------------------
void Declaration::tcheck(Env &env)
{
  Type const *specType = spec->tcheck(env);

  FAKELIST_FOREACH_NC(Declarator, decllist, iter) {
    DeclaratorTcheck dt(specType, dflags);
    iter->tcheck(env, dt);
  }
}


//  -------------------- ASTTypeId -------------------
void ASTTypeId::tcheck(Env &env, bool allowVarArraySize)
{
  Type const *specType = spec->tcheck(env);
  DeclaratorTcheck dt(specType, DF_NONE);
  dt.allowVarArraySize = allowVarArraySize;
  decl->tcheck(env, dt);
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
    return env.error(ret, stringc
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


// we (may) have just encountered some syntax which declares
// some template parameters, but found that the declaration
// matches a prior declaration with (possibly) some other template
// parameters; verify that they match (or complain), and then
// discard the ones stored in the environment (if any)
void verifyCompatibleTemplates(Env &env, CompoundType *prior)
{
  Scope *scope = env.scope();
  if (!scope->templateParams && !prior->templateParams) {
    // neither talks about templates, forget the whole thing
    return;
  }

  if (!scope->templateParams && prior->templateParams) {
    env.error(stringc
      << "prior declaration of " << prior->keywordAndName()
      << " at " << prior->typedefVar->loc
      << " was templatized with parameters "
      << prior->templateParams->toString()
      << " but the this one is not templatized",
      true /*disambiguating*/);
    return;
  }

  if (scope->templateParams && !prior->templateParams) {
    env.error(stringc
      << "prior declaration of " << prior->keywordAndName()
      << " at " << prior->typedefVar->loc
      << " was not templatized, but this one is, with parameters "
      << scope->templateParams->toString(),
      true /*disambiguating*/);
    return;
  }

  // now we know both declarations have template parameters;
  // check them for naming equivalent types (the actual names
  // given to the parameters don't matter; the current
  // declaration's names have already been entered into the
  // template-parameter scope)
  if (scope->templateParams->equalTypes(prior->templateParams)) {
    // ok
  }
  else {
    env.error(stringc
      << "prior declaration of " << prior->keywordAndName()
      << " at " << prior->typedefVar->loc
      << " was templatized with parameters "
      << prior->templateParams->toString()
      << " but this one has parameters "
      << scope->templateParams->toString()
      << ", and these are not equivalent",
      true /*disambiguating*/);
  }
}


Type const *makeNewCompound(CompoundType *&ct, Env &env, StringRef name,
                            SourceLocation const &loc, TypeIntr keyword,
                            bool forward)
{
  ct = new CompoundType((CompoundType::Keyword)keyword, name);
  ct->templateParams = env.takeTemplateParams();
  ct->forward = forward;
  if (name) {
    bool ok = env.addCompound(ct);
    xassert(ok);     // already checked that it was ok
  }

  // make the implicit typedef
  Type const *ret = makeType(ct);
  Variable *tv = new Variable(loc, name, ret, (DeclFlags)(DF_TYPEDEF | DF_IMPLICIT));
  ct->typedefVar = tv;
  if (name) {
    if (!env.addVariable(tv)) {
      // this isn't really an error, because in C it would have
      // been allowed, so C++ does too [ref?]
      //return env.error(stringc
      //  << "implicit typedef associated with " << ct->keywordAndName()
      //  << " conflicts with an existing typedef or variable",
      //  true /*disambiguating*/);
    }
  }

  return ret;
}


Type const *TS_elaborated::tcheck(Env &env)
{
  env.setLoc(loc);

  if (keyword == TI_ENUM) {
    EnumType const *et = env.lookupPQEnum(name);
    if (!et) {
      return env.error(stringc
        << "there is no enum called `" << name << "'",
        true /*disambiguating*/);
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
        return makeNewCompound(ct, env, name->getName(), loc, keyword,
                               true /*forward*/);
      }
    }

    // check that the keywords match; these are different enums,
    // but they agree for the three relevant values
    if ((int)keyword != (int)ct->keyword) {
      return env.error(stringc
        << "you asked for a " << toString(keyword) << " called `"
        << name << "', but that's actually a " << toString(ct->keyword));
    }

    verifyCompatibleTemplates(env, ct);

    return makeType(ct);
  }
}


Type const *TS_classSpec::tcheck(Env &env)
{
  env.setLoc(loc);

  // are we in a template?
  bool inTemplate = env.scope()->templateParams != NULL;

  // see if the environment already has this name
  CompoundType *ct = name? env.lookupCompound(name, true /*innerOnly*/) : NULL;
  Type const *ret;
  if (ct) {
    // check that the keywords match
    if ((int)ct->keyword != (int)keyword) {
      // apparently this isn't an error, because my streambuf.h
      // violates it..
      //return env.error(stringc
      //  << "there is already a " << ct->keywordAndName()
      //  << ", but here you're defining a " << toString(keyword)
      //  << " " << name);
      
      trace("env") << "changing " << ct->keywordAndName()
                   << " to a " << toString(keyword) << endl;
      ct->keyword = (CompoundType::Keyword)keyword;
    }

    // check that the previous was a forward declaration
    if (!ct->forward) {
      return env.error(stringc
        << ct->keywordAndName() << " has already been defined");
    }

    // now it is no longer a forward declaration
    ct->forward = false;

    verifyCompatibleTemplates(env, ct);

    ret = makeType(ct);
  }

  else {
    // no existing compound; make a new one
    ret = makeNewCompound(ct, env, name, loc, keyword, false /*forward*/);
  }

  // look at the base class specifications
  if (bases) {
    FAKELIST_FOREACH(BaseClassSpec, bases, iter) {
      CompoundType *base = env.lookupPQCompound(iter->name);
      if (!base) {
        env.error(stringc
          << "no class called `" << *(iter->name) << "' was found");
      }
      else {                                 
        AccessKeyword acc = iter->access;
        if (acc == AK_UNSPECIFIED) {
          // if the user didn't specify, then apply the default
          // access mode for the inheriting class
          acc = (ct->keyword==CompoundType::K_CLASS? AK_PRIVATE : AK_PUBLIC);
        }
        ct->bases.append(new BaseClass(base, acc, iter->isVirtual));
      }
    }
  }

  bool prevDO = false;
  if (inTemplate) {
    // only report serious errors while checking the class
    // body, in the absence of actual template arguments
    prevDO = env.setDisambiguateOnly(true);
  }

  // open a scope, and install 'ct' as the compound which is
  // being built; in fact, 'ct' itself is a scope, so we use
  // that directly
  env.extendScope(ct);

  // look at members: first pass is to enter them into the environment
  FOREACH_ASTLIST_NC(Member, members->list, iter) {
    iter.data()->tcheck(env);
  }

  // second pass: check function bodies
  FOREACH_ASTLIST_NC(Member, members->list, iter2) {
    if (iter2.data()->isMR_func()) {
      Function *f = iter2.data()->asMR_func()->f;

      // ordinarily we'd complain about seeing two declarations
      // of the same class member, so to tell D_name::itcheck not
      // to complain, this flag says we're in the second pass
      // tcheck of an inline member function
      f->dflags = (DeclFlags)(f->dflags | DF_INLINE_DEFN);

      f->tcheck(env, true /*checkBody*/);
    }
  }

  // now retract the class scope from the stack of scopes; do
  // *not* destroy it!
  env.retractScope(ct);

  if (inTemplate) {
    env.setDisambiguateOnly(prevDO);
  }

  return ret;
}
  

Type const *TS_enumSpec::tcheck(Env &env)
{
  env.setLoc(loc);

  EnumType *et = new EnumType(name);
  Type *ret = makeType(et);

  FAKELIST_FOREACH_NC(Enumerator, elts, iter) {
    iter->tcheck(env, et, ret);
  }

  if (name) {
    env.addEnum(et);

    // make the implicit typedef
    Variable *tv = new Variable(loc, name, ret, (DeclFlags)(DF_TYPEDEF | DF_IMPLICIT));
    et->typedefVar = tv;
    if (!env.addVariable(tv)) {
      // this isn't really an error, because in C it would have
      // been allowed, so C++ does too [ref?]
      //return env.error(stringc
      //  << "implicit typedef associated with enum " << et->name
      //  << " conflicts with an existing typedef or variable",
      //  true /*disambiguating*/);
    }
  }

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
  // mark the function as inline, whether or not the
  // user explicitly did so
  f->dflags = (DeclFlags)(f->dflags | DF_INLINE);

  // we check the bodies in a second pass, after all the class
  // members have been added to the class, so that the potential
  // scope of all class members includes all function bodies
  // [cppstd sec. 3.3.6]
  f->tcheck(env, false /*checkBody*/);
}

void MR_access::tcheck(Env &env)
{
  env.scope()->curAccess = k;
}


// -------------------- Enumerator --------------------
void Enumerator::tcheck(Env &env, EnumType *parentEnum, Type *parentType)
{
  var = new Variable(loc, name, parentType, DF_ENUMERATOR);

  enumValue = parentEnum->nextValue;
  if (expr) {
    expr->tcheck(env);

    // will either set 'enumValue', or print (add) an error message
    expr->constEval(env, enumValue);
  }

  parentEnum->addValue(name, enumValue, var);
  parentEnum->nextValue = enumValue + 1;

  // cppstd sec. 3.3.1:
  //   "The point of declaration for an enumerator is immediately after
  //   its enumerator-definition. [Example:
  //     const int x = 12;
  //     { enum { x = x }; }
  //   Here, the enumerator x is initialized with the value of the
  //   constant x, namely 12. ]"
  if (!env.addVariable(var)) {
    env.error(stringc
      << "enumerator " << name << " conflicts with an existing variable "
      << "or typedef by the same name");
  }
}


// -------------------- Declarator --------------------
void Declarator::tcheck(Env &env, DeclaratorTcheck &dt)
{
  // cppstd sec. 3.4.3 para 3:
  //    "In a declaration in which the declarator-id is a
  //    qualified-id, names used before the qualified-id
  //    being declared are looked up in the defining
  //    namespace scope; names following the qualified-id
  //    are looked up in the scope of the member's class
  //    or namespace."
  //
  // to implement this, I'll find the declarator's qualified
  // scope ahead of time and add it to the scope stack
  PQName const *declaratorId = decl->getDeclaratorId();
  Scope *qualifiedScope = NULL;
  if (declaratorId &&     // i.e. not abstract
      declaratorId->hasQualifiers()) {
    // look up the scope named by the qualifiers
    qualifiedScope = env.lookupQualifiedScope(declaratorId);
    if (!qualifiedScope) {
      // the environment will have already reported the
      // problem; go ahead and check the declarator in the
      // unqualified (normal) scope; it's about the best I
      // could imagine doing as far as error recovery goes
    }
    else {
      // ok, put the scope we found into the scope stack
      // so the declarator's names will get its benefit
      env.extendScope(qualifiedScope);
    }
  }

  // get the variable from the IDeclarator
  decl->tcheck(env, dt);
  var = dt.var;

  if (qualifiedScope) {
    // pull the scope back out of the stack; if this is a
    // declarator attached to a function definition, then
    // Function::tcheck will re-extend it for analyzing
    // the function body
    env.retractScope(qualifiedScope);
  }

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

    // TODO: in the case of class member functions, delay checking
    // the initializer until the entire class body has been scanned

    init->tcheck(env);
  }
}


//  ------------------ IDeclarator ------------------
void IDeclarator::tcheck(Env &env, DeclaratorTcheck &dt)
{
  // apply the stars left to right; the leftmost is the innermost
  FAKELIST_FOREACH_NC(PtrOperator, stars, iter) {
    dt.type = makePtrOperType(iter->isPtr? PO_POINTER : PO_REFERENCE,
                              iter->cv, dt.type);
  }

  // now go inside and apply the specific type constructor
  itcheck(env, dt);
}


// given an unqualified name which might refer to a conversion
// operator, rewrite the type if it is
Type const *makeConversionOperType(Env &env, OperatorName *o,
                                   Type const *spec)
{
  if (!o->isON_conversion()) {
    // no change for non-conversion operators
    return spec;
  }
  else {
    ON_conversion *c = o->asON_conversion();

    c->type->tcheck(env);
    Type const *destType = c->type->getType();

    // need a function which returns 'destType', but has the
    // other characteristics gathered into 'spec'; make sure
    // 'spec' is a function type
    if (!spec->isFunctionType()) {
      env.error("conversion operator must be a function");
      return spec;
    }
    FunctionType const *specFunc = &( spec->asFunctionTypeC() );

    if (specFunc->params.isNotEmpty() || specFunc->acceptsVarargs) {
      env.error("conversion operator cannot accept arguments");
      return spec;
    }

    // now build another function type using specFunc's cv flags
    // (since we've verified none of the other info is interesting);
    // in particular, fill in the conversion destination type as
    // the function's return type
    FunctionType *ft = new FunctionType(destType, specFunc->cv);
    ft->templateParams = env.takeTemplateParams();

    return ft;
  }
}


// This function is perhaps the most complicated in this entire
// module.  It has the responsibility of adding a variable called
// 'name', with type 'spec', to the environment.  But to do this it
// has to implement the various rules for when declarations conflict,
// overloading, qualified name lookup, etc.
void D_name_itcheck(Env &env, DeclaratorTcheck &dt,
                    SourceLocation const &loc, PQName const *name)
{
  // this is used to refer to a pre-existing declaration of the same
  // name; I moved it up top so my error subroutines can use it
  Variable *prior = NULL;

  // the unqualified part of 'name', mapped if necessary for
  // constructor names
  StringRef unqualifiedName = name? name->getName() : NULL;

  goto realStart;

  // This code has a number of places where very different logic paths
  // lead to the same conclusion.  So, I'm going to put the code for
  // these conclusions up here (like mini-subroutines), and 'goto'
  // them when appropriate.  I put them at the top instead of the
  // bottom since g++ doesn't like me to jump forward over variable
  // declarations.

  makeDummyVar:
  {
    // the purpose of this is to allow the caller to have a workable
    // object, so we can continue making progress diagnosing errors
    // in the program; this won't be entered in the environment, even
    // though the 'name' is not NULL
    dt.var = new Variable(loc, unqualifiedName, dt.type, dt.dflags);

    // a bit of error recovery: if we clashed with a prior declaration,
    // and that one was in a named scope, then make our fake variable
    // also appear to be in that scope (this helps for parsing
    // constructor definitions, even when the declarator has a type
    // clash)
    if (prior && prior->scope) {
      dt.var->scope = prior->scope;
    }

    return;
  }

  declarationTypeMismatch:
  {
    // this message reports two declarations which declare the same
    // name, but their types are different; we only jump here *after*
    // ruling out the possibility of function overloading
    env.error(dt.type, stringc
      << "prior declaration of `" << *name << "' had type `"
      << prior->type->toString() << "', but this one uses `"
      << dt.type->toString() << "'");
    goto makeDummyVar;
  }

realStart:
  if (!name) {
    // no name, nothing to enter in environment
    dt.var = new Variable(loc, NULL, dt.type, dt.dflags);
    return;
  }

  if (name->getUnqualifiedName()->isPQ_operator()) {
    // conversion operators require that I play some games
    // with the type
    //
    // this may not be perfect, because it makes looking up
    // the set of candidate conversion functions difficult
    // (you have to explicitly iterate through the base classes)
    dt.type = makeConversionOperType(
      env, name->getUnqualifiedName()->asPQ_operatorC()->o, dt.type);
  }

  else if (dt.type->isCDtorFunction()) {
    // the return type claim's it's a constructor or destructor;
    // validate that
    CompoundType *ct = env.scope()->curCompound;
    if (!ct) {
      env.error("cannot have ctors or dtors outside of a class",
                true /*disambiguates*/);
      goto makeDummyVar;
    }
      
    // get the claimed class name so we can compare to the name
    // of the class whose scope we're in
    char const *className = unqualifiedName;
    char const *fnKind = "constructor";
    if (className[0] == '~') {          // dtor
      className++;
      fnKind = "destructor";
    }

    // can't rely on string table for comparison because 'className'
    // might be pointing into the middle of one the table's strings
    if (0!=strcmp(ct->name, className)) {
      env.error(stringc
        << fnKind << " `" << *name << "' not match the name `"
        << ct->name << "', the class in whose scope it appears",
        true /*disambiguates*/);
      goto makeDummyVar;
    }

    if (unqualifiedName[0] != '~') {    // ctor
      // ok, last bit of housekeeping: if I just use the class name
      // as the name of the constructor, then that will hide the
      // class's name as a type, which messes everything up.  so,
      // I'll kludge together another name for constructors (one
      // which the C++ programmer can't type) and just make sure I
      // always look up constructors under that name
      unqualifiedName = env.constructorSpecialName;
    }
  }

  // are we in a class member list?  we can't be in a member
  // list if the name is qualified (and if it's qualified then
  // a class scope has been pushed, so we'd be fooled)
  CompoundType *enclosingClass =
    name->hasQualifiers()? NULL : env.scope()->curCompound;

  // if we're not in a class member list, and the type is not a
  // function type, and 'extern' is not specified, then this is
  // a definition
  if (!enclosingClass &&
      !dt.type->isFunctionType() &&
      !(dt.dflags & DF_EXTERN)) {
    dt.dflags = (DeclFlags)(dt.dflags | DF_DEFINITION);
  }

  // has this variable already been declared?
  //Variable *prior = NULL;    // moved to the top

  if (name->hasQualifiers()) {
    // the name has qualifiers, which means it *must* be declared
    // somewhere; now, Declarator::tcheck will have already pushed the
    // qualified scope, so we just look up the name in the now-current
    // environment, which will include that scope
    prior = env.lookupVariable(unqualifiedName, true /*innerOnly*/);
    if (!prior) {
      env.error(stringc
        << "undeclared identifier `" << *name << "'");
      goto makeDummyVar;
    }

    // ok, so we found a prior declaration; but if it's a member of
    // an overload set, then we need to pick the right one now for
    // several reasons:
    //   - the DF_DEFINITION flag is per-member, not per-set
    //   - below we'll be checking for type equality again
    if (prior->overload) {
      OverloadSet *set = prior->overload;
      prior = NULL;     // for now we haven't found a valid prior decl
      SMUTATE_EACH_OBJLIST(Variable, set->set, iter) {
        if (iter.data()->type->equals(dt.type)) {
          // ok, this is the right one
          prior = iter.data();
          break;
        }
      }

      if (!prior) {
        env.error(dt.type, stringc
          << "the name `" << *name << "' is overloaded, but the type `"
          << dt.type->toString() << "' doesn't match any of the "
          << set->set.count() << " declared overloaded instances");
        goto makeDummyVar;
      }
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
    prior = env.lookupVariable(unqualifiedName, true /*innerOnly*/);
  }

  // check for overloading
  OverloadSet *overloadSet = NULL;    // null until valid overload seen
  if (!name->hasQualifiers() &&
      prior &&
      prior->type->isFunctionType() &&
      dt.type->isFunctionType() &&
      !prior->type->equals(dt.type)) {
    // potential overloading situation; get the two function types
    FunctionType const *priorFt = &( prior->type->asFunctionTypeC() );
    FunctionType const *specFt = &( dt.type->asFunctionTypeC() );

    // (BUG: this isn't the exact criteria for allowing overloading,
    // but it's close)
    if (
         // always allow the overloading for conversion operators
         (unqualifiedName != env.conversionOperatorName) &&

         // make sure the parameter lists are not the same
         (priorFt->equalParameterLists(specFt))
       ) {
      // figure out *what* was different, so the error message can
      // properly tell the user
      if (!priorFt->retType->equals(specFt->retType)) {
        env.error(stringc
          << "cannot overload `" << *name << "' on return type only");
        goto makeDummyVar;
      }
      else {
        // this probably should be an error message more like "this
        // definition conflicts with the previous" instead of mentioning
        // the overload possibility, so..
        goto declarationTypeMismatch;
      }
    }

    else {
      // ok, allow the overload
      trace("ovl") << "overloaded `" << prior->name
                   << "': `" << prior->type->toString()
                   << "' and `" << dt.type->toString() << "'\n";
      overloadSet = prior->getOverloadSet();
      prior = NULL;    // so we don't consider this to be the same
    }
  }

  // if this gets set, we'll replace a conflicting variable
  // when we got to do the insertion
  bool forceReplace = false;

  // did we find something?
  if (prior) {
    // check for violation of the One Definition Rule
    if (prior->hasFlag(DF_DEFINITION) &&
        (dt.dflags & DF_DEFINITION)) {
      env.error(stringc
        << "duplicate definition for `" << *name
        << "'; previous at " << prior->loc.toString());
      goto makeDummyVar;
    }

    // check for violation of rule disallowing multiple
    // declarations of the same class member; cppstd sec. 9.2:
    //   "A member shall not be declared twice in the
    //   member-specification, except that a nested class or member
    //   class template can be declared and then later defined."
    //
    // I have a specific exception for this when I do the second
    // pass of typechecking for inline members (the code doesn't
    // violate the rule, it only appears to because of the second
    // pass); this exception is indicated by DF_INLINE_DEFN.
    if (enclosingClass && !(dt.dflags & DF_INLINE_DEFN)) {
      env.error(stringc
        << "duplicate member declaration of `" << *name
        << "' in " << enclosingClass->keywordAndName()
        << "; previous at " << prior->loc.toString());
      goto makeDummyVar;
    }

    // check that the types match
    if (!prior->type->equals(dt.type)) {
      // if the previous guy was an implicit typedef, then as a
      // special case allow it, and arrange for the environment
      // to replace the implicit typedef with the variable being
      // declared here
      if (prior->hasFlag(DF_IMPLICIT)) {
        trace("env") << "replacing implicit typedef of " << prior->name
                     << " at " << prior->loc << " with new decl at "
                     << loc << endl;
        forceReplace = true;
        goto noPriorDeclaration;
      }
      else {
        goto declarationTypeMismatch;
      }
    }

    // ok, use the prior declaration, but update the 'loc'
    // if this is the definition
    if (dt.dflags & DF_DEFINITION) {
      trace("odr") << "def'n at " << loc.toString()
                   << " overrides decl at " << prior->loc.toString()
                   << endl;
      prior->loc = loc;
      prior->setFlag(DF_DEFINITION);
      prior->clearFlag(DF_EXTERN);
    }

    dt.var = prior;
    return;
  }

noPriorDeclaration:
  // no prior declaration, make a new variable and put it
  // into the environment (see comments in Declarator::tcheck
  // regarding point of declaration)
  dt.var = new Variable(loc, unqualifiedName, dt.type, dt.dflags);
  if (overloadSet) {
    // don't add it to the environment (another overloaded version
    // is already in the environment), instead add it to the overload set
    overloadSet->addMember(dt.var);
    dt.var->overload = overloadSet;

    // but tell the environment about it, because the environment
    // takes care of making sure that variables' 'scope' field is
    // set correctly..
    env.registerVariable(dt.var);
  }
  else if (!dt.type->isError()) {
    env.addVariable(dt.var, forceReplace);
  }
 
  return;
}

void D_name::itcheck(Env &env, DeclaratorTcheck &dt)
{
  env.setLoc(loc);

  D_name_itcheck(env, dt, loc, name);
}


void D_func::itcheck(Env &env, DeclaratorTcheck &dt)
{
  env.setLoc(loc);

  FunctionType *ft = new FunctionType(dt.type, cv);
  ft->templateParams = env.takeTemplateParams();

  // make a new scope for the parameter list
  Scope *paramScope = env.enterScope();

  // typecheck and add the parameters
  FAKELIST_FOREACH_NC(ASTTypeId, params, iter) {
    iter->tcheck(env);
    Variable *v = iter->decl->var;

    // TODO: record the default argument somewhere

    ft->addParam(new Parameter(v->name, v->type, v));
  }

  env.exitScope(paramScope);

  if (exnSpec) {
    ft->exnSpec = exnSpec->tcheck(env);
  }

  // now that we've constructed this function type, pass it as
  // the 'base' on to the next-lower declarator
  dt.type = ft;
  base->tcheck(env, dt);
}


void D_array::itcheck(Env &env, DeclaratorTcheck &dt)
{
  ArrayType *at;
  if (!size) {
    at = new ArrayType(dt.type);
  }
  else {
    size->tcheck(env);

    int sz;
    if (!dt.allowVarArraySize && 
        size->constEval(env, sz)) {
      at = new ArrayType(dt.type, sz);
    }
    else {
      // error has already been reported, this is a recovery action,
      // *or* we're in E_new and don't look for a constant size
      at = new ArrayType(dt.type  /*as if no size specified*/);
    }
  }

  dt.type = at;
  base->tcheck(env, dt);
}


void D_bitfield::itcheck(Env &env, DeclaratorTcheck &dt)
{
  env.setLoc(loc);

  // check that the expression is a compile-time constant
  int n;
  if (!bits->constEval(env, n)) {
    env.error("bitfield size must be a constant",
              false /*disambiguates*/);
  }

  // TODO: record the size of the bit field somewhere; but
  // that size doesn't influence type checking very much, so
  // fixing this will be a low priority for some time.  I think
  // the way to do it is to make another kind of Type which
  // stacks a bitfield size on top of another Type, and
  // construct such an animal here.

  D_name_itcheck(env, dt, loc, name);
}


// PtrOperator

// ------------------- ExceptionSpec --------------------
FunctionType::ExnSpec *ExceptionSpec::tcheck(Env &env)
{
  FunctionType::ExnSpec *ret = new FunctionType::ExnSpec;
  
  FAKELIST_FOREACH_NC(ASTTypeId, types, iter) {
    iter->tcheck(env);
    ret->types.append(iter->getType());
  }

  return ret;
}


// ------------------ OperatorDeclarator ----------------
char const *ON_newDel::getOperatorName() const
{
  return (isNew && isArray)? "operator-new[]" :
         (isNew && !isArray)? "operator-new" :
         (!isNew && isArray)? "operator-delete[]" :
                              "operator-delete";
}

char const *ON_binary::getOperatorName() const
{
  switch (op) {
    default:              xfailure("bad code");
    case BIN_EQUAL:       return "operator==";
    case BIN_NOTEQUAL:    return "operator!=";
    case BIN_LESS:        return "operator<";
    case BIN_GREATER:     return "operator>";
    case BIN_LESSEQ:      return "operator<=";
    case BIN_GREATEREQ:   return "operator>=";
    case BIN_MULT:        return "operator*";
    case BIN_DIV:         return "operator/";
    case BIN_MOD:         return "operator%";
    case BIN_PLUS:        return "operator+";
    case BIN_MINUS:       return "operator-";
    case BIN_LSHIFT:      return "operator<<";
    case BIN_RSHIFT:      return "operator>>";
    case BIN_BITAND:      return "operator&";
    case BIN_BITXOR:      return "operator^";
    case BIN_BITOR:       return "operator|";
    case BIN_AND:         return "operator&&";
    case BIN_OR:          return "operator||";
    case BIN_ASSIGN:      return "operator=";
    case BIN_DOT_STAR:    return "operator.*";
    case BIN_ARROW_STAR:  return "operator->*";
    case BIN_IMPLIES:     return "operator==>";
  };
}

char const *ON_unary::getOperatorName() const
{
  switch (op) {
    default:           xfailure("bad code");
    case UNY_NOT:      return "operator!";
    case UNY_BITNOT:   return "operator~";
  }
}

char const *ON_effect::getOperatorName() const
{
  switch (op) {
    default:            xfailure("bad code");
    case EFF_PREINC:    return "operator++";
    case EFF_PREDEC:    return "operator--";
  }
}

char const *ON_assign::getOperatorName() const
{
  switch (op) {
    default:            xfailure("bad code");
    case BIN_ASSIGN:    return "operator=";
    case BIN_MULT:      return "operator*=";
    case BIN_DIV:       return "operator/=";
    case BIN_MOD:       return "operator%=";
    case BIN_PLUS:      return "operator+=";
    case BIN_MINUS:     return "operator-=";
    case BIN_LSHIFT:    return "operator<<=";
    case BIN_RSHIFT:    return "operator>>=";
    case BIN_BITAND:    return "operator&=";
    case BIN_BITXOR:    return "operator^=";
    case BIN_BITOR:     return "operator|=";
    case BIN_AND:       return "operator&&=";
    case BIN_OR:        return "operator||=";
  }
}

char const *ON_overload::getOperatorName() const
{
  switch (op) {
    default:             xfailure("bad code");
    case OVL_COMMA:      return "operator,";
    case OVL_ARROW:      return "operator->";
    case OVL_PARENS:     return "operator()";
    case OVL_BRACKETS:   return "operator[]";
  }
}

char const *ON_conversion::getOperatorName() const
{                   
  // this is the sketchy one..
  return "conversion-operator";
}


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
    env.errors.append(new ErrorMsg(
      "following messages from an ambiguity", false /*isWarning*/,
      loc, false /*disambiguates*/));
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
    env.error("more than one ambiguous alternative succeeds",
              false /*disambiguates*/);
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
  env.error("unknown statement ambiguity", false /*disambiguates*/);
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
  Scope *scope = env.enterScope();

  FOREACH_ASTLIST_NC(Statement, stmts, iter) {
    // have to potentially change the list nodes themselves
    iter.setDataLink( iter.data()->tcheck(env) );
  }

  env.exitScope(scope);
}


void S_if::itcheck(Env &env)
{
  // if 'cond' declares a variable, its scope is the
  // body of the "if"
  Scope *scope = env.enterScope();

  cond->tcheck(env);
  thenBranch = thenBranch->tcheck(env);
  elseBranch = elseBranch->tcheck(env);

  env.exitScope(scope);
}


void S_switch::itcheck(Env &env)
{
  Scope *scope = env.enterScope();

  cond->tcheck(env);
  branches = branches->tcheck(env);

  env.exitScope(scope);
}


void S_while::itcheck(Env &env)
{
  Scope *scope = env.enterScope();

  cond->tcheck(env);
  body = body->tcheck(env);

  env.exitScope(scope);
}


void S_doWhile::itcheck(Env &env)
{
  body = body->tcheck(env);
  expr = expr->tcheck(env);

  // TODO: verify that 'expr' makes sense in a boolean context
}


void S_for::itcheck(Env &env)
{
  Scope *scope = env.enterScope();

  init = init->tcheck(env);
  cond->tcheck(env);
  after = after->tcheck(env);
  body = body->tcheck(env);

  env.exitScope(scope);
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
  Scope *scope = env.enterScope();

  typeId->tcheck(env);
  body->tcheck(env);

  env.exitScope(scope);
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
  
  // elaborate the AST by storing the computed type, *unless*
  // we're only disambiguating (because in that case many of
  // the types will be ST_ERROR anyway)
  //if (!env.onlyDisambiguating()) {
  //  type = t;
  //}
  //
  // update: made it unconditional again because after tcheck()
  // the callers expect to be able to dig in and find the type;
  // I guess I'll at some point have to write a visitor to
  // clear the computed types if I want to actually check the
  // template bodies after arguments are presented

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


Type const *makeLvalType(Type const *underlying)
{
  if (underlying->isLval()) {
    // this happens for example if a variable is declared to
    // a reference type
    return underlying; 
  }
  else {
    return makeRefType(underlying);
  }
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
    return makeLvalType(var->type);
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
    return env.error(func->type, stringc
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
    return env.error(rt, stringc
      << "non-compound `" << rt->toString()
      << "' doesn't have fields to access");
  }

  if (fieldName->hasQualifiers()) {
    // this also catches destructor invocations
    return env.unimp("fields with qualified names");
  }

  // make sure the type has been completed
  if (!ct->isComplete()) {
    return env.error(rt, stringc
      << "attempt to access a field of incomplete type "
      << ct->keywordAndName());
  }

  // look for the named field
  field = ct->getNamedFieldC(fieldName->getName(), env);
  if (!field) {
    return env.error(rt, stringc
      << "there is no field called `" << fieldName->getName()
      << "' in " << obj->type->toString());
  }

  // type of expression is type of field; possibly as an lval
  if (obj->type->isLval() &&
      !field->type->isFunctionType()) {
    return makeLvalType(field->type);
  }
  else {
    return field->type;
  }
}


Type const *E_sizeof::itcheck(Env &env)
{
  expr = expr->tcheck(env);
  
  // TODO: this will fail an assertion if someone asks for the
  // size of a variable of template-type-parameter type..
  size = expr->type->reprSize();

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
    return env.error(expr->type, stringc
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
    return env.error(rt, stringc
      << "cannot derefence non-pointer `" << rt->toString() << "'");
  }
  PointerType const &pt = rt->asPointerTypeC();
  xassert(pt.op == PO_POINTER);   // otherwise not rval!

  // dereferencing yields an lvalue
  return makeLvalType(pt.atType);
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
  size = atype->getType()->reprSize();

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
  
  atype->tcheck(env, true /*allowVarArraySize*/);
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
    env.error(t, stringc
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
  return env.type_info_const_ref;
}


Type const *E_typeidType::itcheck(Env &env)
{
  type->tcheck(env);
  return env.type_info_const_ref;
}


// --------------------- Expression constEval ------------------
bool Expression::constEval(Env &env, int &result) const
{
  xassert(!ambiguity);

  ASTSWITCHC(Expression, this) {
    ASTCASEC(E_boolLit, b)
      result = b->b? 1 : 0;
      return true;

    ASTNEXTC(E_intLit, i)
      result = i->i;
      return true;
      
    ASTNEXTC(E_charLit, c)
      result = c->c;
      return true;

    ASTNEXTC(E_variable, v)
      if (v->var->hasFlag(DF_ENUMERATOR)) {
        // this is an enumerator; find the corresponding
        // enum type, and look up the name to find the value
        EnumType const &et = v->var->type->asCVAtomicTypeC().atomic->asEnumTypeC();
        EnumType::Value const *val = et.getValue(v->var->name);
        xassert(val);    // otherwise the type information is wrong..
        result = val->value;
        return true;
      }

      // TODO: add capability to look up 'int const' variables
      // (the problem for the moment is I don't have a way to
      // navigate from a Variable to an initializing expression)

      env.error(stringc
        << "can't const-eval non-const variable `" << v->var->name << "'",
        false /*disambiguates*/);
      return false;

    ASTNEXTC(E_sizeof, s)
      result = s->size;
      return true;

    ASTNEXTC(E_unary, u)
      if (!u->expr->constEval(env, result)) return false;
      switch (u->op) {
        default: xfailure("bad code");
        case UNY_PLUS:   result = +result;  return true;
        case UNY_MINUS:  result = -result;  return true;
        case UNY_NOT:    result = !result;  return true;
        case UNY_BITNOT: result = ~result;  return true;
      }

    ASTNEXTC(E_binary, b)
      int v1, v2;
      if (!b->e1->constEval(env, v1) ||
          !b->e2->constEval(env, v2)) return false;

      if (v2==0 && (b->op == BIN_DIV || b->op == BIN_MOD)) {
        env.error("division by zero in constant expression",
                  false /*disambiguates*/);
        return false;
      }

      switch (b->op) {
        default: xfailure("bad code");

        case BIN_EQUAL:     result = (v1 == v2);  return true;
        case BIN_NOTEQUAL:  result = (v1 != v2);  return true;
        case BIN_LESS:      result = (v1 < v2);  return true;
        case BIN_GREATER:   result = (v1 > v2);  return true;
        case BIN_LESSEQ:    result = (v1 <= v2);  return true;
        case BIN_GREATEREQ: result = (v1 >= v2);  return true;

        case BIN_MULT:      result = (v1 * v2);  return true;
        case BIN_DIV:       result = (v1 / v2);  return true;
        case BIN_MOD:       result = (v1 % v2);  return true;
        case BIN_PLUS:      result = (v1 + v2);  return true;
        case BIN_MINUS:     result = (v1 - v2);  return true;
        case BIN_LSHIFT:    result = (v1 << v2);  return true;
        case BIN_RSHIFT:    result = (v1 >> v2);  return true;
        case BIN_BITAND:    result = (v1 & v2);  return true;
        case BIN_BITXOR:    result = (v1 ^ v2);  return true;
        case BIN_BITOR:     result = (v1 | v2);  return true;
        case BIN_AND:       result = (v1 && v2);  return true;
        case BIN_OR:        result = (v1 || v2);  return true;
      }

    ASTNEXTC(E_cast, c)
      if (!c->expr->constEval(env, result)) return false;

      Type const *t = c->ctype->getType();
      if (t->isIntegerType()) {
        return true;       // ok
      }
      else {
        // TODO: this is probably not the right rule..
        env.error(stringc
          << "in constant expression, can only cast to integer types, not `"
          << t->toString() << "'");
        return false;
      }

    ASTNEXTC(E_cond, c)
      if (!c->cond->constEval(env, result)) return false;

      if (result) {
        return c->th->constEval(env, result);
      }
      else {
        return c->el->constEval(env, result);
      }

    ASTNEXTC(E_comma, c)
      return c->e2->constEval(env, result);

    ASTNEXTC(E_sizeofType, s)
      result = s->size;
      return true;

    ASTDEFAULTC
      env.error(stringc <<
        kindName() << " is not constEval'able",
        false /*disambiguates*/);
      return false;

    ASTENDCASEC
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

// -------------------- TemplateDeclaration ---------------
void TemplateDeclaration::tcheck(Env &env)
{
  // make a new scope to hold the template parameters
  Scope *paramScope = env.enterScope();
    
  // make a list of template parameters
  TemplateParams *tparams = new TemplateParams;

  // check each of the parameters, i.e. enter them into the scope
  FAKELIST_FOREACH_NC(TemplateParameter, params, iter) {
    iter->tcheck(env, tparams);
  }

  // mark the new scope as unable to accept new names, so
  // that the function or class being declared will be entered
  // into the scope above us
  paramScope->canAcceptNames = false;

  // put the template parameters in a place the D_func will find them
  paramScope->templateParams = tparams;

  // in what follows, ignore errors that are not disambiguating
  //bool prev = env.setDisambiguateOnly(true);
  // 
  // update: moved this inside Function::tcheck and TS_classSpec::tcheck
  // so that the declarators would still get full checking

  // check the particular declaration
  itcheck(env);

  // restore prior error mode
  //env.setDisambiguateOnly(prev);

  // remove the template argument scope
  env.exitScope(paramScope);
}


void TD_func::itcheck(Env &env)
{
  // check the function definition; internally this will get
  // the template parameters attached to the function type
  f->tcheck(env, true /*checkBody*/);
}


void TD_class::itcheck(Env &env)
{ 
  // check the class definition; it knows what to do about
  // the template parameters (just like for functions)
  type->tcheck(env);
}


void TP_type::tcheck(Env &env, TemplateParams *tparams)
{
  // std 14.1 is a little unclear about whether the type
  // name is visible to its own default argument; but that
  // would make no sense, so I'm going to check the
  // default type first
  if (defaultType) {
    defaultType->tcheck(env);
  }

  if (!name) {
    // nothing to do for anonymous parameters
    return;
  }

  // the standard is not clear about whether the user code should
  // be able to do this:
  //   template <class T>
  //   int f(class T &t)      // can use "class T" instead of just "T"?
  //   { ... }
  // my approach of making a TypeVariable, instead of calling
  // it a CompoundType with a flag for 'is type variable', rules
  // out the subsequent use of "class T" ...

  // make a type variable for this thing
  TypeVariable *type = new TypeVariable(name);

  // introduce 'name' into the environment as a typedef for the
  // type variable
  Type const *fullType = makeType(type);
  Variable *var = new Variable(loc, name, fullType, DF_TYPEDEF);
  type->typedefVar = var;
  if (!env.addVariable(var)) {
    env.error(stringc
      << "duplicate template parameter `" << name << "'",
      false /*disambiguates*/);
  }

  // add this parameter to the list of them
  tparams->params.append(new Parameter(name, fullType, var));
}


