// cc_env.cc            see license.txt for copyright and terms of use
// code for cc_env.h

#include "cc_env.h"      // this module
#include "trace.h"       // tracingSys
#include "ckheap.h"      // heapCheck
#include "strtable.h"    // StringTable
#include "cc_lang.h"     // CCLang


// ----------------- ErrorMsg -----------------
ErrorMsg::~ErrorMsg()
{}


string ErrorMsg::toString() const
{
  return stringc << ::toString(loc)
                 << (isWarning? ": warning: " : ": error: ")
                 << msg;
}


// --------------------- Env -----------------
Env::Env(StringTable &s, CCLang &L, TypeFactory &tf)
  : scopes(),
    disambiguateOnly(false),
    anonTypeCounter(1),
    disambiguationNestingLevel(0),
    errors(),
    str(s),
    lang(L),
    tfac(tf),
    madeUpVariables(),

    // filled in below; initialized for definiteness
    type_info_const_ref(NULL),

    conversionOperatorName(NULL),
    constructorSpecialName(NULL),
    functionOperatorName(NULL),
    thisName(NULL),

    special_getStandardConversion(NULL),
    special_getImplicitConversion(NULL),
    special_testOverload(NULL),

    dependentTypeVar(NULL),
    dependentVar(NULL),
    errorTypeVar(NULL),
    errorVar(NULL)
{
  // slightly less verbose
  #define HERE HERE_SOURCELOC

  // create first scope
  SourceLoc emptyLoc = SL_UNKNOWN;
  {
    Scope *s = new Scope(0 /*changeCount*/, emptyLoc);
    scopes.prepend(s);

    // cause Variables inserted into this scope to acquire DF_GLOBAL
    s->isGlobalScope = true;
  }

  // create the typeid type
  CompoundType *ct = tfac.makeCompoundType(CompoundType::K_CLASS, str("type_info"));
  // TODO: put this into the 'std' namespace
  // TODO: fill in the proper fields and methods
  type_info_const_ref = tfac.makeRefType(HERE,
    makeCVAtomicType(HERE, ct, CV_CONST));

  // some special names; pre-computed (instead of just asking the
  // string table for it each time) because in certain situations
  // I compare against them frequently; the main criteria for
  // choosing these names is they have to be untypable by the C++
  // programmer (i.e. if the programmer types one it won't be
  // lexed as a single name)
  conversionOperatorName = str("conversion-operator");
  constructorSpecialName = str("constructor-special");
  functionOperatorName = str("operator()");
  thisName = str("this");

  dependentTypeVar = makeVariable(HERE, str("<dependentTypeVar>"),
                                  getSimpleType(HERE, ST_DEPENDENT), DF_TYPEDEF);
  madeUpVariables.append(dependentTypeVar);

  dependentVar = makeVariable(HERE, str("<dependentVar>"),
                              getSimpleType(HERE, ST_DEPENDENT), DF_NONE);
  madeUpVariables.append(dependentVar);

  errorTypeVar = makeVariable(HERE, str("<errorTypeVar>"),
                              getSimpleType(HERE, ST_ERROR), DF_TYPEDEF);
  madeUpVariables.append(errorTypeVar);

  // this is *not* a typedef, because I use it in places that I
  // want something to be treated as a variable, not a type
  errorVar = makeVariable(HERE, str("<errorVar>"),
                          getSimpleType(HERE, ST_ERROR), DF_NONE);
  madeUpVariables.append(errorVar);

  // create declarations for some built-in operators
  // [cppstd 3.7.3 para 2]
  Type *t_void = getSimpleType(HERE, ST_VOID);
  Type *t_voidptr = makePtrType(HERE, t_void);

  // note: my stddef.h typedef's size_t to be 'int', so I just use
  // 'int' directly here instead of size_t
  Type *t_size_t = getSimpleType(HERE, ST_INT);

  // but I do need a class called 'bad_alloc'..
  //   class bad_alloc;
  CompoundType *dummyCt;
  Type *t_bad_alloc =
    makeNewCompound(dummyCt, scope(), str("bad_alloc"), HERE,
                    TI_CLASS, true /*forward*/);

  // void* operator new(std::size_t sz) throw(std::bad_alloc);
  declareFunction1arg(t_voidptr, "operator new",
                      t_size_t, "sz",
                      t_bad_alloc);

  // void* operator new[](std::size_t sz) throw(std::bad_alloc);
  declareFunction1arg(t_voidptr, "operator new[]",
                      t_size_t, "sz",
                      t_bad_alloc);

  // void operator delete (void *p) throw();
  declareFunction1arg(t_void, "operator delete",
                      t_voidptr, "p",
                      t_void);

  // void operator delete[] (void *p) throw();
  declareFunction1arg(t_void, "operator delete[]",
                      t_voidptr, "p",
                      t_void);

  // for GNU compatibility
  // void *__builtin_next_arg(void *p);
  declareFunction1arg(t_voidptr, "__builtin_next_arg",
                      t_voidptr, "p",
                      NULL /*exnType*/);

  // for testing various modules
  special_getStandardConversion = declareSpecialFunction("__getStandardConversion");
  special_getImplicitConversion = declareSpecialFunction("__getImplicitConversion");
  special_testOverload = declareSpecialFunction("__testOverload");

  #undef HERE
}

Env::~Env()
{
  // delete the scopes one by one, so we can skip any
  // which are in fact not owned
  while (scopes.isNotEmpty()) {
    Scope *s = scopes.removeFirst();
    if (s->curCompound) {
      // this isn't one we own
    }
    else {
      // we do own this one
      delete s;
    }
  }

  errors.deleteAll();
}


void Env::declareFunction1arg(Type *retType, char const *funcName,
                              Type *arg1Type, char const *arg1Name,
                              Type * /*nullable*/ exnType)
{
  // clone the types so client analyses can treat them independently
  retType  = tfac.cloneType(retType);
  arg1Type = tfac.cloneType(arg1Type);
  if (exnType) {
    exnType = tfac.cloneType(exnType);
  }

  FunctionType *ft = makeFunctionType(HERE_SOURCELOC, retType);
  Variable *p = makeVariable(HERE_SOURCELOC, str(arg1Name), arg1Type, DF_NONE);
  // 'p' doesn't get added to 'madeUpVariables' because it's not toplevel,
  // and it's reachable through 'var' (below)

  ft->addParam(p);
  if (exnType) {
    ft->exnSpec = new FunctionType::ExnSpec;

    // slightly clever hack: say "throw()" by saying "throw(void)"
    if (!exnType->isSimple(ST_VOID)) {
      ft->exnSpec->types.append(exnType);
    }
  }
  ft->doneParams();

  Variable *var = makeVariable(HERE_SOURCELOC, str(funcName), ft, DF_NONE);
  addVariable(var);
  madeUpVariables.append(var);
}


// this declares a function that accepts any # of arguments,
// and returns 'void'
StringRef Env::declareSpecialFunction(char const *name)
{                                                     
  Type *t_void = getSimpleType(HERE_SOURCELOC, ST_VOID);
  FunctionType *ft = makeFunctionType(HERE_SOURCELOC, t_void);
  ft->flags |= FF_VARARGS;
  ft->doneParams();

  StringRef ret = str(name);
  Variable *var = makeVariable(HERE_SOURCELOC, ret, ft, DF_NONE);
  addVariable(var);
  madeUpVariables.append(var);                                   
  
  return ret;
}

  
FunctionType *Env::makeDestructorFunctionType(SourceLoc loc)
{
  FunctionType *ft = makeFunctionType(loc, getSimpleType(loc, ST_CDTOR));
  ft->doneParams();
  return ft;
}


Scope *Env::enterScope(char const *forWhat)
{
  trace("env") << locStr() << ": entered scope for " << forWhat << "\n";

  // propagate the 'curFunction' field
  Function *f = scopes.first()->curFunction;
  Scope *newScope = new Scope(getChangeCount(), loc());
  scopes.prepend(newScope);
  newScope->curFunction = f;

  return newScope;
}

void Env::exitScope(Scope *s)
{
  trace("env") << locStr() << ": exited scope\n";
  Scope *f = scopes.removeFirst();
  xassert(s == f);
  delete f;
}


void Env::extendScope(Scope *s)
{
  if (s->curCompound) {
    trace("env") << locStr() << ": extending scope "
                 << s->curCompound->keywordAndName() << "\n";
  }
  else {
    trace("env") << locStr() << ": extending scope at "
                 << (void*)s << "\n";
  }
  Scope *prevScope = scope();
  scopes.prepend(s);
  s->curLoc = prevScope->curLoc;
}

void Env::retractScope(Scope *s)
{
  if (s->curCompound) {
    trace("env") << locStr() << ": retracting scope "
                 << s->curCompound->keywordAndName() << "\n";
  }
  else {
    trace("env") << locStr() << ": retracting scope at " 
                 << (void*)s << "\n";
  }
  Scope *first = scopes.removeFirst();
  xassert(first == s);
  // we don't own 's', so don't delete it
}


#if 0   // does this even work?
CompoundType *Env::getEnclosingCompound()
{
  MUTATE_EACH_OBJLIST(Scope, scopes, iter) {
    if (iter.data()->curCompound) {
      return iter.data()->curCompound;
    }
  }
  return NULL;
}
#endif // 0


void Env::setLoc(SourceLoc loc)
{
  TRACE("loc", "setLoc: " << toString(loc));
  
  // only set it if it's a valid location; I get invalid locs
  // from abstract declarators..
  if (loc != SL_UNKNOWN) {
    Scope *s = scope();
    s->curLoc = loc;
  }
}

SourceLoc Env::loc() const
{
  return scopeC()->curLoc;
}


// -------- insertion --------
Scope *Env::acceptingScope()
{
  Scope *s = scopes.first();    // first in list
  if (s->canAcceptNames) {
    return s;    // common case
  }

  s = scopes.nth(1);            // second in list
  if (s->canAcceptNames) {
    return s;
  }
                                                                   
  // since non-accepting scopes should always be just above
  // an accepting scope
  xfailure("had to go more than two deep to find accepting scope");
  return NULL;    // silence warning
}


Scope *Env::outerScope()
{
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    Scope *s = iter.data();
    if (!s->canAcceptNames) continue;        // skip template scopes
    if (s->curCompound) continue;            // skip class scopes
    if (s->isParameterListScope ) continue;  // skip parameter list scopes
    
    return s;
  }

  xfailure("couldn't find the outer scope!");
  return NULL;    // silence warning
}


Scope *Env::enclosingScope()
{
  // inability to accept names doesn't happen arbitrarily,
  // and we shouldn't run into it here

  Scope *s = scopes.nth(0);
  xassert(s->canAcceptNames);

  s = scopes.nth(1);
  if (!s->canAcceptNames) {
    error("did you try to make a templatized anonymous union (!), "
          "or am I confused?");
    return scopes.nth(2);   // error recovery..
  }

  return s;
}


bool Env::addVariable(Variable *v, bool forceReplace)
{
  if (disambErrorsSuppressChanges()) {
    // the environment is not supposed to be modified by an ambiguous
    // alternative that fails
    trace("env") << "not adding variable `" << v->name
                 << "' because there are disambiguating errors\n";
    return true;    // don't cause further errors; pretend it worked
  }

  Scope *s = acceptingScope();
  registerVariable(v);
  if (!s->addVariable(v, forceReplace)) {
    return false;
  }

  return true;
}

void Env::registerVariable(Variable *v)
{
  Scope *s = acceptingScope();
  s->registerVariable(v);
}


bool Env::addCompound(CompoundType *ct)
{
  // like above
  if (disambErrorsSuppressChanges()) {
    trace("env") << "not adding compound `" << ct->name
                 << "' because there are disambiguating errors\n";
    return true;
  }

  return acceptingScope()->addCompound(ct);
}


bool Env::addEnum(EnumType *et)
{
  // like above
  if (disambErrorsSuppressChanges()) {
    trace("env") << "not adding enum `" << et->name
                 << "' because there are disambiguating errors\n";
    return true;
  }

  return acceptingScope()->addEnum(et);
}


// -------- lookup --------
Scope *Env::lookupQualifiedScope(PQName const *name)
{
  bool dummy1, dummy2;
  return lookupQualifiedScope(name, dummy1, dummy2);
}

Scope *Env::lookupQualifiedScope(PQName const *name, 
                                 bool &dependent, bool &anyTemplates)
{
  // this scope keeps track of which scope we've identified
  // so far, given how many qualifiers we've processed;
  // initially it is NULL meaning we're still at the default,
  // lexically-enclosed scope
  Scope *scope = NULL;

  do {
    PQ_qualifier const *qualifier = name->asPQ_qualifierC();

    // get the first qualifier
    StringRef qual = qualifier->qualifier;
    if (!qual) {
      // this is a reference to the global scope, i.e. the scope
      // at the bottom of the stack
      scope = scopes.last();

      // should be syntactically impossible to construct bare "::"
      // with template arguments
      xassert(!qualifier->targs);
    }

    else {
      // look for a class called 'qual' in scope-so-far; look in the
      // *variables*, not the *compounds*, because it is legal to make
      // a typedef which names a scope, and use that typedef'd name as
      // a qualifier
      //
      // however, this still is not quite right, see cppstd 3.4.3 para 1
      // update: LF_ONLY_TYPES now gets it right, I think
      Variable *qualVar =
        scope==NULL? lookupVariable(qual, LF_ONLY_TYPES) :
                     scope->lookupVariable(qual, *this, LF_ONLY_TYPES);
      if (!qualVar) {
        // I'd like to include some information about which scope
        // we were looking in, but I don't want to be computing
        // intermediate scope names for successful lookups; also,
        // I am still considering adding some kind of scope->name()
        // functionality, which would make this trivial.
        //
        // alternatively, I could just re-traverse the original name;
        // I'm lazy for now
        error(stringc
          << "cannot find scope name `" << qual << "'",
          true /*disambiguating*/);
        return NULL;
      }

      if (!qualVar->hasFlag(DF_TYPEDEF)) {
        error(stringc
          << "variable `" << qual << "' used as if it were a scope name");
        return NULL;
      }

      // check for a special case: a qualifier that refers to
      // a template parameter
      if (qualVar->type->isTypeVariable()) {
        // we're looking inside an uninstantiated template parameter
        // type; the lookup fails, but no error is generated here
        dependent = true;     // tell the caller what happened
        return NULL;
      }
        
      // the const_cast here is unfortunate, but I don't see an
      // easy way around it
      CompoundType *ct =
        const_cast<CompoundType*>(qualVar->type->ifCompoundType());
      if (!ct) {
        error(stringc
          << "typedef'd name `" << qual << "' doesn't refer to a class, "
          << "so it can't be used as a scope qualifier");
        return NULL;
      }

      if (ct->isTemplate()) {
        anyTemplates = true;
      }

      // check template argument compatibility
      if (qualifier->targs) {
        if (!ct->isTemplate()) {
          error(stringc
            << "class `" << qual << "' isn't a template");
          // recovery: use the scope anyway
        }   

        else {
          // TODO: maybe put this back in
          //ct = checkTemplateArguments(ct, qualifier->targs);
        }
      }

      else if (ct->isTemplate()) {
        error(stringc
          << "class `" << qual
          << "' is a template, you have to supply template arguments");
        // recovery: use the scope anyway
      }

      // TODO: actually check that there are the right number
      // of arguments, of the right types, etc.

      // now that we've found it, that's our active scope
      scope = ct;
    }

    // advance to the next name in the sequence
    name = qualifier->rest;
  } while (name->hasQualifiers());

  return scope;
}


#if 0     // aborted implementation for now
// verify correspondence between template arguments and
// template parameters; return selected specialization of 'ct'
CompoundType *Env::
  checkTemplateArguments(CompoundType *ct, FakeList<TempalteArgument> const *args)
{
  ClassTemplateInfo *tinfo = ct->templateInfo;
  xassert(tinfo);

  // check argument list against parameter list
  TemplateArgument const *argIter = args->firstC();
  SFOREACH_OBJLIST(Variable, tinfo->params, paramIter) {
    if (!argIter) {
      error("not enough arguments to template");
      return NULL;
    }

    // for now, since I only have type-valued parameters
    // and type-valued arguments, correspondence is automatic

    argIter = argIter->next;
  }
  if (argIter) {
    error("too many arguments to template");
    return NULL;
  }

  // check for specializations
  CompoundType *special = selectSpecialization(tinfo, args);
  if (!special) {
    return ct;         // no specialization
  }
  else {
    return special;    // specialization found
  }
}

// check the list of specializations in 'tinfo' for one that
// matches the given arguments in 'args'; return the matching
// specialization, or NULL if none matches
CompoundType *Env::
  selectSpecialization(ClassTemplateInfo *tinfo, FakeList<TempalteArgument> const *args)
{
  SFOREACH_OBJLIST_NC(CompoundType, tinfo->specializations, iter) {
    CompoundType *special = iter.data();

    // create an empty unification environment
    TypeUnificationEnv tuEnv;
    bool matchFailure = false;

    // check supplied argument list against specialized argument list
    TemplateArgument const *userArg = args->firstC();
    FAKELIST_FOREACH(TemplateArgument,
                     special->templateInfo->specialArguments,
                     specialArg) {
      // the specialization cannot have more arguments than
      // the client code supplies; TODO: make sure that the
      // code which accepts specializations enforces this
      xassert(argIter);

      // get the types for each
      Type *userType = userArg->asTA_typeC()->getType();
      Type *specialType = specialArg->asTA_typeC()->getType();

      // unify them
      if (!tuEnv.unify(userType, specialType)) {
        matchFailure = true;
        break;
      }

      userArg = userArg->next;
    }

    if (userArg) {
      // we didn't use all of the user's arguments.. I guess this
      // is a partial specialization of some sort, and it matches
    }

    if (!matchFailure) {
      // success
      return special;

      // TODO: try all the specializations, and complain if more
      // than once succeeds
    }
  }

  // no matching specialization
  return NULL;
}
#endif // 0, aborted implementation


Variable *Env::lookupPQVariable(PQName const *name, LookupFlags flags)
{
  Variable *var;

  Scope *scope;      // scope in which name is found, if it is
  if (name->hasQualifiers()) {
    // look up the scope named by the qualifiers
    bool dependent = false, anyTemplates = false;
    scope = lookupQualifiedScope(name, dependent, anyTemplates);
    if (!scope) {
      if (dependent) {
        // tried to look into a template parameter
        if (flags & LF_TYPENAME) {
          return dependentTypeVar;    // user claims it's a type
        }
        else {
          return dependentVar;        // user makes no claim, so it's a variable
        }
      }
      else if (anyTemplates) {
        // maybe failed due to incompleteness of specialization implementation
        return errorVar;
      }
      else {
        // error has already been reported
        return NULL;
      }
    }

    // look inside the final scope for the final name
    var = scope->lookupVariable(name->getName(), *this, flags);
    if (!var) {
      if (anyTemplates) {
        // maybe failed due to incompleteness of specialization implementation;
        // since I'm returning an error variable, it means I'm guessing that
        // this thing names a variable and not a type
        return errorVar;
      }

      error(stringc
        << name->qualifierString() << " has no member called `"
        << name->getName() << "'",
        false /*disambiguating*/);
      return NULL;
    }
  }

  else {
    var = lookupVariable(name->getName(), flags, scope);
  }

  if (var) {
    // get the final component of the name, so we can inspect
    // whether it has template arguments
    PQName const *final = name->getUnqualifiedNameC();

    // compare the name's template status to whether template
    // arguments were supplied; note that you're only *required*
    // to pass arguments for template classes, since template
    // functions can infer their arguments
    if (var->isTemplateClass()) {
      if (!final->isPQ_template()) {
        // cppstd 14.6.1 para 1: if the name refers to a template
        // in whose scope we are, then it need not have arguments
        CompoundType *enclosing = findEnclosingTemplateCalled(final->getName());
        if (enclosing) {
          trace("template") << "found bare reference to enclosing template: "
                            << enclosing->name << "\n";
          return enclosing->typedefVar;
        }

        // this disambiguates
        //   new Foo< 3 > +4 > +5;
        // which could absorb the '3' as a template argument or not,
        // depending on whether Foo is a template
        error(stringc
          << "`" << var->name << "' is a template, but template "
          << "arguments were not supplied",
          true /*disambiguating*/);
        return NULL;
      }
      else {
        // apply the template arguments to yield a new type based
        // on the template
        return instantiateClassTemplate(scope, var->type->asCompoundType(),
                                        final->asPQ_templateC()->args);
      }
    }
    else if (!var->isTemplate() &&
             final->isPQ_template()) {
      // disambiguates the same example as above, but selects
      // the opposite interpretation
      error(stringc
        << "`" << var->name << "' is not a template, but template "
        << "arguments were supplied",
        true /*disambiguating*/);
      return NULL;
    }
  }

  return var;
}

Variable *Env::lookupVariable(StringRef name, LookupFlags flags)
{
  Scope *dummy;
  return lookupVariable(name, flags, dummy);
}

Variable *Env::lookupVariable(StringRef name, LookupFlags flags,
                              Scope *&foundScope)
{
  if (flags & LF_INNER_ONLY) {
    // here as in every other place 'innerOnly' is true, I have
    // to skip non-accepting scopes since that's not where the
    // client is planning to put the name
    foundScope = acceptingScope();
    return foundScope->lookupVariable(name, *this, flags);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    Variable *v = iter.data()->lookupVariable(name, *this, flags);
    if (v) {
      foundScope = iter.data();
      return v;
    }
  }
  return NULL;    // not found
}

CompoundType *Env::lookupPQCompound(PQName const *name, LookupFlags flags)
{
  // same logic as for lookupPQVariable
  if (name->hasQualifiers()) {
    Scope *scope = lookupQualifiedScope(name);
    if (!scope) return NULL;

    CompoundType *ret = scope->lookupCompound(name->getName(), flags);
    if (!ret) {
      error(stringc
        << name->qualifierString() << " has no class/struct/union called `"
        << name->getName() << "'",
        true /*disambiguating*/);
      return NULL;
    }

    return ret;
  }

  return lookupCompound(name->getName(), flags);
}

CompoundType *Env::lookupCompound(StringRef name, LookupFlags flags)
{
  if (flags & LF_INNER_ONLY) {
    return acceptingScope()->lookupCompound(name, flags);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    CompoundType *ct = iter.data()->lookupCompound(name, flags);
    if (ct) {
      return ct;
    }
  }
  return NULL;    // not found
}

EnumType *Env::lookupPQEnum(PQName const *name, LookupFlags flags)
{
  // same logic as for lookupPQVariable
  if (name->hasQualifiers()) {
    Scope *scope = lookupQualifiedScope(name);
    if (!scope) return NULL;

    EnumType *ret = scope->lookupEnum(name->getName(), flags);
    if (!ret) {
      error(stringc
        << name->qualifierString() << " has no enum called `"
        << name->getName() << "'",
        true /*disambiguating*/);
      return NULL;
    }

    return ret;
  }

  return lookupEnum(name->getName(), flags);
}

EnumType *Env::lookupEnum(StringRef name, LookupFlags flags)
{
  if (flags & LF_INNER_ONLY) {
    return acceptingScope()->lookupEnum(name, flags);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    EnumType *et = iter.data()->lookupEnum(name, flags);
    if (et) {
      return et;
    }
  }
  return NULL;    // not found
}


TemplateParams * /*owner*/ Env::takeTemplateParams()
{
  Scope *s = scope();
  TemplateParams *ret = s->curTemplateParams;
  s->curTemplateParams = NULL;
  return ret;
}


StringRef Env::getAnonName(TypeIntr keyword)
{
  // construct a name
  string name = stringc << "Anon_" << toString(keyword) 
                        << "_" << anonTypeCounter++;
                     
  // map to a stringref
  StringRef sr = str(name);

  // any chance this name already exists?
  if (lookupVariable(sr)) {
    return getAnonName(keyword);    // try again
  }
  else {
    return sr;
  }
}


ClassTemplateInfo *Env::takeTemplateClassInfo(StringRef baseName)
{
  ClassTemplateInfo *ret = NULL;

  Scope *s = scope();
  if (s->curTemplateParams) {
    ret = new ClassTemplateInfo(baseName);
    ret->params.concat(s->curTemplateParams->params);
    delete takeTemplateParams();
  }

  return ret;
}


Type *Env::makeNewCompound(CompoundType *&ct, Scope * /*nullable*/ scope,
                           StringRef name, SourceLoc loc,
                           TypeIntr keyword, bool forward)
{
  ct = tfac.makeCompoundType((CompoundType::Keyword)keyword, name);

  // transfer template parameters
  ct->templateInfo = takeTemplateClassInfo(name);

  ct->forward = forward;
  if (name && scope) {
    bool ok = scope->addCompound(ct);
    xassert(ok);     // already checked that it was ok
  }

  // make the implicit typedef
  Type *ret = makeType(loc, ct);
  Variable *tv = makeVariable(loc, name, ret, (DeclFlags)(DF_TYPEDEF | DF_IMPLICIT));
  ct->typedefVar = tv;
  if (name && scope) {
    if (!scope->addVariable(tv)) {
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


// ----------------- template instantiation -------------
Variable *Env::instantiateClassTemplate
  (Scope *foundScope, CompoundType *base, 
   FakeList<TemplateArgument> *arguments, CompoundType *inst)
{
  // go over the list of arguments, and make a list of
  // semantic arguments
  SObjList<STemplateArgument> sargs;
  {
    FAKELIST_FOREACH_NC(TemplateArgument, arguments, iter) {
      if (iter->sarg.hasValue()) {
        sargs.append(&iter->sarg);
      }
      else {
        // this happens frequently when processing the uninstantiated
        // template, but should then be suppressed in that case
        error("attempt to use unresolved arguments to instantiate a class");
        return dependentTypeVar;
      }
    }
  }

  // has this class already been instantiated?
  if (!inst) {
    SFOREACH_OBJLIST(CompoundType, base->templateInfo->instantiations, iter) {
      if (iter.data()->templateInfo->equalArguments(sargs)) {
        // found it
        Variable *ret = iter.data()->typedefVar;
        xassert(ret);    // I've had a few instances of failing to make this,
                         // and if it looks like lookup failed if it's NULL
        return ret;
      }
    }
  }

  // render the template arguments into a string that we can use
  // as the name of the instantiated class; my plan is *not* that
  // this string serve as the unique ID, but rather that it be
  // a debugging aid only
  StringRef instName = str.add(stringc << base->name << sargsToString(sargs));
  trace("template") << (base->forward? "(forward) " : "")
                    << "instantiating class: " << instName << endl;

  // remove scopes from the environment until the innermost
  // scope on the scope stack is the same one that the template
  // definition appeared in; template definitions can see names
  // visible from their defining scope only [cppstd 14.6 para 1]
  ObjList<Scope> innerScopes;
  Scope *argScope = NULL;
  if (!base->forward) {      // don't mess with it if not going to tcheck anything
    while (scopes.first() != foundScope) {
      innerScopes.prepend(scopes.removeFirst());
    }

    // make a new scope for the template arguments
    argScope = enterScope("template argument bindings");

    // simultaneously iterate over the parameters and arguments,
    // creating bindings from params to args
    SObjListIter<Variable> paramIter(base->templateInfo->params);
    FakeList<TemplateArgument> *argIter(arguments);
    while (!paramIter.isDone() && argIter) {
      Variable const *param = paramIter.data();
      TemplateArgument *arg = argIter->first();

      if (param->hasFlag(DF_TYPEDEF) &&
          arg->isTA_type()) {
        // bind the type parameter to the type argument; I considered
        // simply modifying 'param' but decided this would be cleaner
        // for now..
        Variable *binding = makeVariable(param->loc, param->name,
                                         arg->asTA_type()->type->getType(),
                                         DF_TYPEDEF);
        addVariable(binding);
      }
      else if (!param->hasFlag(DF_TYPEDEF) &&
               arg->isTA_nontype()) {
        // TODO: verify that the argument in fact matches the parameter type

        // bind the nontype parameter directly to the nontype expression;
        // this will suffice for checking the template body, because I
        // can const-eval the expression whenever it participates in
        // type determination; the type must be made 'const' so that
        // E_variable::constEval will believe it can evaluate it
        Type *bindType = tfac.applyCVToType(param->loc, CV_CONST,
                                            param->type, NULL /*syntax*/);
        Variable *binding = makeVariable(param->loc, param->name,
                                         bindType, DF_NONE);
        binding->value = arg->asTA_nontype()->expr;
        addVariable(binding);
      }
      else {                 
        // mismatch between argument kind and parameter kind
        char const *paramKind = param->hasFlag(DF_TYPEDEF)? "type" : "non-type";
        char const *argKind = arg->isTA_type()? "type" : "non-type";
        error(stringc
          << "`" << param->name << "' is a " << paramKind
          << " parameter, but `" << arg->argString() << "' is a "
          << argKind << " argument");
      }

      paramIter.adv();
      argIter = argIter->butFirst();
    }

    if (!paramIter.isDone()) {
      error(stringc
        << "too few template arguments to `" << base->name
        << "' (and partial specialization is not implemented)");
    }
    else if (argIter) {
      error(stringc
        << "too many template arguments to `" << base->name << "'");
    }
  }

  // make a copy of the template definition body AST
  TS_classSpec *copy = base->forward? NULL : base->syntax->clone();
  if (copy && tracingSys("cloneAST")) {
    cout << "--------- clone of " << base->name << " ------------\n";
    copy->debugPrint(cout, 0);
  }

  // make the CompoundType that will represent the instantiated class,
  // if we don't already have one
  if (!inst) {
    // 1/21/03: I had been using 'instName' as the class name, but that
    // causes problems when trying to recognize constructors
    inst = tfac.makeCompoundType(base->keyword, base->name);
    inst->instName = instName;     // stash it here instead
    inst->forward = base->forward;

    // copy over the template arguments so we can recognize this
    // instantiation later
    inst->templateInfo = new ClassTemplateInfo(base->name);
    {
      SFOREACH_OBJLIST(STemplateArgument, sargs, iter) {
        inst->templateInfo->arguments.append(new STemplateArgument(*iter.data()));
      }

      // remember the argument syntax too, in case this is a
      // forward declaration
      inst->templateInfo->argumentSyntax = arguments;
    }

    // tell the base template about this instantiation; this has to be
    // done before invoking the type checker, to handle cases where the
    // template refers to itself recursively (which is very common)
    base->templateInfo->instantiations.append(inst);

    // wrap the compound in a regular type
    SourceLoc copyLoc = copy? copy->loc : SL_UNKNOWN;
    Type *type = makeType(copyLoc, inst);

    // make a fake implicit typedef; this class and its typedef won't
    // actually appear in the environment directly, but making the
    // implicit typedef helps ensure uniformity elsewhere; also must be
    // done before type checking since it's the typedefVar field which
    // is returned once the instantiation is found via 'instantiations'
    Variable *var = makeVariable(copyLoc, instName, type,
                                 DF_TYPEDEF | DF_IMPLICIT);
    inst->typedefVar = var;
  }

  if (copy) {
    // invoke the TS_classSpec typechecker, giving to it the
    // CompoundType we've built to contain its declarations; for
    // now I don't support nested template instantiation
    copy->ctype = inst;
    copy->tcheckIntoCompound(*this, DF_NONE, inst, false /*inTemplate*/,
                             NULL /*containingClass*/);

    if (tracingSys("cloneTypedAST")) {
      cout << "--------- typed clone of " << base->name << " ------------\n";
      copy->debugPrint(cout, 0);
    }

    // remove the argument scope
    exitScope(argScope);

    // re-add the inner scopes removed above
    while (innerScopes.isNotEmpty()) {
      scopes.prepend(innerScopes.removeFirst());
    }
  }

  return inst->typedefVar;
}


// given a name that was found without qualifiers or template arguments,
// see if we're currently inside the scope of a template definition
// with that name
CompoundType *Env::findEnclosingTemplateCalled(StringRef name)
{
  FOREACH_OBJLIST(Scope, scopes, iter) {
    Scope const *s = iter.data();

    if (s->curCompound &&
        s->curCompound->templateInfo &&
        s->curCompound->templateInfo->baseName == name) {
      return s->curCompound;
    }
  }
  return NULL;     // not found
}


void Env::instantiateForwardClasses(Scope *scope, CompoundType *base)
{
  SFOREACH_OBJLIST_NC(CompoundType, base->templateInfo->instantiations, iter) {
    CompoundType *inst = iter.data();
    xassert(inst->templateInfo);

    if (inst->forward) {
      trace("template") << "instantiating previously forward " << inst->name << "\n";
      inst->forward = false;
      instantiateClassTemplate(scope, base, inst->templateInfo->argumentSyntax, 
                               inst /*use this one*/);
    }
    else {
      // this happens in e.g. t0079.cc, when the template becomes
      // an instantiation of itself because the template body
      // refers to itself with template arguments supplied
      
      // update: maybe not true anymore?
    }
  }
}


// -------- diagnostics --------
Type *Env::error(SourceLoc L, char const *msg, bool disambiguates)
{
  trace("error") << (disambiguates? "[d] " : "") << "error: " << msg << endl;
  if (!disambiguateOnly || disambiguates) {
    errors.prepend(new ErrorMsg(
      stringc << msg, false /*isWarning*/, L, disambiguates));
  }
  return getSimpleType(SL_UNKNOWN, ST_ERROR);
}

Type *Env::error(char const *msg, bool disambiguates)
{
  return error(loc(), msg, disambiguates);
}


Type *Env::warning(char const *msg)
{
  trace("error") << "warning: " << msg << endl;
  if (!disambiguateOnly) {
    errors.prepend(new ErrorMsg(
      stringc << msg, true /*isWarning*/, loc(), false /*disambiguates*/));
  }
  return getSimpleType(SL_UNKNOWN, ST_ERROR);
}


Type *Env::unimp(char const *msg)
{
  // always print this immediately, because in some cases I will
  // segfault (deref'ing NULL) right after printing this
  cout << "unimplemented: " << msg << endl;

  errors.prepend(new ErrorMsg(
    stringc << "unimplemented: " << msg, false /*isWarning*/, loc(), false /*disambiguates*/));
  return getSimpleType(SL_UNKNOWN, ST_ERROR);
}


Type *Env::error(Type *t, char const *msg)
{
  if (t->isSimple(ST_DEPENDENT)) {
    // no report, propagate dependentness
    return t;
  }

  // TODO: remove containsTypeVariables() from this..
  if (t->containsErrors() ||
      t->containsTypeVariables()) {   // hack until template stuff fully working
    // no report
    return getSimpleType(SL_UNKNOWN, ST_ERROR);
  }
  else {
    // report; clashes never disambiguate
    return error(msg, false /*disambiguates*/);
  }
}


bool Env::setDisambiguateOnly(bool newVal)
{
  bool ret = disambiguateOnly;
  disambiguateOnly = newVal;
  return ret;
}


STATICDEF bool Env::listHasDisambErrors(ObjList<ErrorMsg> const &list)
{
  FOREACH_OBJLIST(ErrorMsg, list, iter) {
    if (iter.data()->disambiguates) {
      return true;     // has at least one disambiguating error
    }
  }
  return false;        // no disambiguating errors
}


void Env::checkFuncAnnotations(FunctionType *, D_func *)
{}

void Env::addedNewCompound(CompoundType *)
{}


// EOF
