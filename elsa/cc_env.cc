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
  return stringc << loc.toString() << " " << msg;
}


// --------------------- Env -----------------
Env::Env(StringTable &s, CCLang &L, TypeFactory &tf)
  : scopes(),
    disambiguateOnly(false),
    anonTypeCounter(1),
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

    dependentTypeVar(NULL),
    errorVar(NULL)
{
  // create first scope
  SourceLocation emptyLoc;
  scopes.prepend(new Scope(0 /*changeCount*/, emptyLoc));

  // create the typeid type
  CompoundType *ct = new CompoundType(CompoundType::K_CLASS, str("type_info"));
  // TODO: put this into the 'std' namespace
  // TODO: fill in the proper fields and methods
  type_info_const_ref = tfac.makeRefType(makeCVAtomicType(ct, CV_CONST));

  // some special names; pre-computed (instead of just asking the
  // string table for it each time) because in certain situations
  // I compare against them frequently; the main criteria for
  // choosing these names is they have to be untypable by the C++
  // programmer (i.e. if the programmer types them they won't be
  // lexed as single names)
  conversionOperatorName = str("conversion-operator");
  constructorSpecialName = str("constructor-special");
  functionOperatorName = str("operator()");

  dependentTypeVar = makeVariable(HERE_SOURCELOC, str("<dependentTypeVar>"),
                                  getSimpleType(ST_DEPENDENT), DF_TYPEDEF);
  madeUpVariables.append(dependentTypeVar);

  // this is *not* a typedef, because I use it in places that I
  // want something to be treated as a variable, not a type
  errorVar = makeVariable(HERE_SOURCELOC, str("<errorVar>"),
                          getSimpleType(ST_ERROR), DF_NONE);
  madeUpVariables.append(errorVar);

  // create declarations for some built-in operators
  // [cppstd 3.7.3 para 2]
  Type *t_void = getSimpleType(ST_VOID);
  Type *t_voidptr = makePtrType(t_void);

  // note: my stddef.h typedef's size_t to be 'int', so I use
  // 'int' directly here instead of size_t
  Type *t_int = getSimpleType(ST_INT);

  // but I do need a class called 'bad_alloc'..
  //   class bad_alloc;
  CompoundType *dummyCt;
  Type *t_bad_alloc =
    makeNewCompound(dummyCt, scope(), str("bad_alloc"), HERE_SOURCELOC,
                    TI_CLASS, true /*forward*/);

  // void* operator new(std::size_t sz) throw(std::bad_alloc);
  declareFunction1arg(t_voidptr, "operator new",
                      t_int, "sz",
                      t_bad_alloc);

  // void* operator new[](std::size_t sz) throw(std::bad_alloc);
  declareFunction1arg(t_voidptr, "operator new[]",
                      t_int, "sz",
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
                              Type *exnType)
{
  FunctionType *ft = makeFunctionType(retType, CV_NONE);
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


Scope *Env::enterScope()
{
  trace("env") << locStr() << ": entered scope\n";

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


void Env::setLoc(SourceLocation const &loc)
{
  trace("loc") << "setLoc: " << loc.toString() << endl;
  
  // only set it if it's a valid location; I get invalid locs
  // from abstract declarators..
  if (loc.validLoc()) {
    Scope *s = scope();
    s->curLoc = loc;
  }
}

SourceLocation const &Env::loc() const
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
  return acceptingScope()->addCompound(ct);
}


bool Env::addEnum(EnumType *et)
{
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
          << "cannot find scope name `" << qual << "' for `" << *name << "'",
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

  if (name->hasQualifiers()) {
    // look up the scope named by the qualifiers
    bool dependent = false, anyTemplates = false;
    Scope *scope = lookupQualifiedScope(name, dependent, anyTemplates);
    if (!scope) {
      if (dependent) {
        // tried to look into a template parameter
        return dependentTypeVar;
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
    var = lookupVariable(name->getName(), flags);
  }

  if (var &&
      name->getUnqualifiedName()->isPQ_template()) {
    // make sure the name in question is a template
    if (var->type->isTemplateFunction() &&
        !var->hasFlag(DF_TYPEDEF)) {
      // ok; a template function
    }
    else if (var->type->isTemplateClass() &&
             var->hasFlag(DF_TYPEDEF)) {
      // ok; a typedef referring to a template class
      #if 0    // disabling template instantiation for now..
      // instantiate it
      CompoundType *instantiatedCt
        = instantiateClass(var->type->ifCompoundType(),
                           name->getUnqualifiedName()->asPQ_templateC()->args);

      // get the typedef variable for it
      var = instantiatedCt->typedefVar;
      #endif // 0
    }
    else {
      // I'm not sure if I really need to make this disambiguating
      // (i.e. I don't know if there is a syntax example which is
      // ambiguous without this error), but since I should always
      // know when something is or is not a template (even when
      // processing template code itself) this shouldn't introduce any
      // problems.
      error(stringc
        << "`" << *name << "' does not refer to a template",
        true /*disambiguates*/);
      return NULL;
    }
  }

  return var;
}

Variable *Env::lookupVariable(StringRef name, LookupFlags flags)
{
  if (flags & LF_INNER_ONLY) {
    // here as in every other place 'innerOnly' is true, I have
    // to skip non-accepting scopes since that's not where the
    // client is planning to put the name
    return acceptingScope()->lookupVariable(name, *this, flags);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    Variable *v = iter.data()->lookupVariable(name, *this, flags);
    if (v) {
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


// given 'tclass', the type of the uninstantiated template, and 'args'
// which specifies the template arguments to use for instantiation,
// create an instantiated type and associated typedef Variable, and
// return the type
CompoundType *Env::instantiateClass(
  CompoundType *tclass, FakeList<TemplateArgument> *args)
{
  // uniformity with other code..
  Env &env = *this;

  // construct what the instantiated name would be
  stringBuilder instName;
  {
    instName << tclass->name << "<";

    int ct=0;
    FAKELIST_FOREACH(TemplateArgument, args, iter) {
      if (ct++ > 0) {
        instName << ",";
      }
      instName << iter->argString();
    }

    instName << ">";
  }
  StringRef instNameRef = str(instName);

  // has this class already been instantiated?
  CompoundType *ret = tclass->templateInfo->instantiated.queryif(instNameRef);
  if (ret) return ret;

  // make a new class, and register it as an instantiation
  ret = new CompoundType(tclass->keyword, instNameRef);
  tclass->templateInfo->instantiated.add(instNameRef, ret);

  // clone the abstract syntax of the template class, substituting
  // the constructed name for the original
  ret->syntax = tclass->syntax->clone();  
  ret->syntax->name = new PQ_name(instNameRef);
    
  if (tracingSys("printClonedAST")) {     
    cout << "---------- cloned: " << instNameRef << " ----------\n";
    ret->syntax->debugPrint(cout, 0);
  }

  // begin working through the template parameters so we know what
  // name to associated with the template arguments when we bind them
  SObjListIter<Variable> paramIter(tclass->templateInfo->params);

  // create a new scope, and insert the template argument bindings
  Scope *argScope = enterScope();
  FAKELIST_FOREACH(TemplateArgument, args, iter) {
    TA_type const *arg = iter->asTA_typeC();

    if (paramIter.isDone()) {
      env.error(stringc
        << instNameRef << " does not supply enough template arguments; "
        << tclass->name << " wants "
        << tclass->templateInfo->params.count() << " arguments");
    cleanup:
      exitScope(argScope);
      return NULL;   // leaves 'ret' half-baked.. should be ok
    }
    Variable const *param = paramIter.data();

    if (!param->type->isTypeVariable()) {
      env.unimp("non-class template parameters");
      goto cleanup;
    }

    // make a variable that typedef's the argument type to be
    // the parameter name
    Variable *argVar
      = makeVariable(param->loc, param->name,
                     arg->type->getType(), DF_TYPEDEF);
    if (!argScope->addVariable(argVar)) {
      // I actually think this can't happen because I'd have
      // already detected this problem..
      env.error(stringc
        << "duplicate parameter name `" << param->name << "'");
    }
  }

  // forward-declare 'instNameRef' so the tcheck below will
  // use the CompoundType object we've been working on
  argScope->addCompound(ret);

  // now typecheck the cloned AST syntax; this will create a
  // compound called 'instNameRef', which will match the one
  // we've already forward-declared, so it will use the same
  // type object
  ret->syntax->tcheck(env, DF_NONE);

  if (tracingSys("printTypedClonedAST")) {
    cout << "---------- cloned & typed: " << instNameRef << " ----------\n";
    ret->syntax->debugPrint(cout, 0);
  }

  // ok, we're done: the template class 'tclass' now has a
  // map the created type, and we can return that created type
  // for use by the caller
  env.exitScope(argScope);

  // one more thing: even though it won't be entered into
  // the environment, callers like to have Variables that
  // stand for types, so make one
  ret->typedefVar
    = makeVariable(tclass->typedefVar->loc, instNameRef,
                   makeType(ret), DF_TYPEDEF);

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


ClassTemplateInfo *Env::takeTemplateClassInfo()
{
  ClassTemplateInfo *ret = NULL;

  Scope *s = scope();
  if (s->curTemplateParams) {
    ret = new ClassTemplateInfo;
    ret->params.concat(s->curTemplateParams->params);
    delete takeTemplateParams();
  }

  return ret;
}


Type *Env::makeNewCompound(CompoundType *&ct, Scope *scope,
                           StringRef name, SourceLocation const &loc,
                           TypeIntr keyword, bool forward)
{
  ct = new CompoundType((CompoundType::Keyword)keyword, name);

  // transfer template parameters
  ct->templateInfo = takeTemplateClassInfo();

  ct->forward = forward;
  if (name) {
    bool ok = scope->addCompound(ct);
    xassert(ok);     // already checked that it was ok
  }

  // make the implicit typedef
  Type *ret = makeType(ct);
  Variable *tv = makeVariable(loc, name, ret, (DeclFlags)(DF_TYPEDEF | DF_IMPLICIT));
  ct->typedefVar = tv;
  if (name) {
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


// -------- diagnostics --------
Type *Env::error(char const *msg, bool disambiguates)
{
  trace("error") << (disambiguates? "[d] " : "") << "error: " << msg << endl;
  if (!disambiguateOnly || disambiguates) {
    errors.prepend(new ErrorMsg(
      stringc << "error: " << msg, false /*isWarning*/, loc(), disambiguates));
  }
  return getSimpleType(ST_ERROR);
}


Type *Env::warning(char const *msg)
{
  trace("error") << "warning: " << msg << endl;
  if (!disambiguateOnly) {
    errors.prepend(new ErrorMsg(
      stringc << "warning: " << msg, true /*isWarning*/, loc(), false /*disambiguates*/));
  }
  return getSimpleType(ST_ERROR);
}


Type *Env::unimp(char const *msg)
{
  // always print this immediately, because in some cases I will
  // segfault (deref'ing NULL) right after printing this
  cout << "unimplemented: " << msg << endl;

  errors.prepend(new ErrorMsg(
    stringc << "unimplemented: " << msg, false /*isWarning*/, loc(), false /*disambiguates*/));
  return getSimpleType(ST_ERROR);
}


Type *Env::error(Type *t, char const *msg)
{
  if (t->isSimple(ST_DEPENDENT)) {
    // no report, propagate dependentness
    return t;
  }

  if (t->containsErrors() ||
      t->containsTypeVariables()) {   // hack until template stuff fully working
    // no report
    return getSimpleType(ST_ERROR);
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


#if 0
CVAtomicType *Env::makeCVAtomicType(AtomicType *atomic, CVFlags cv)
  { return tfactory.makeCVAtomicType(atomic, cv); }

PointerType *Env::makePointerType(PtrOper op, CVFlags cv, Type *atType)
  { return tfactory.makePointerType(op, cv, atType); }

FunctionType *Env::makeFunctionType(Type *retType, CVFlags cv)
  { return tfactory.makeFunctionType(retType, cv); }

ArrayType *Env::makeArrayType(Type *eltType, int size)
  { return tfactory.makeArrayType(eltType, size); }

Type *Env::applyCVToType(CVFlags cv, Type *baseType)
  { return tfactory.applyCVToType(cv, baseType); }

ArrayType *Env::setArraySize(ArrayType *type, int size)
  { return tfactory.setArraySize(type, size); }

Type *Env::makeRefType(Type *underlying)
  { return tfactory.makeRefType(underlying); }

PointerType *Env::syntaxPointerType(
  PtrOper op, CVFlags cv, Type *underlying, D_pointer *syntax)
  { return tfactory.syntaxPointerType(op, cv, underlying, syntax); }

FunctionType *Env::syntaxFunctionType(
  Type *retType, CVFlags cv, D_function *syntax)
  { return tfactory.syntaxFunctionType(

PointerType *Env::makeTypeOf_this(
  CompoundType *classType, FunctionType *methodType);


Type *Env::cloneType(Type *src)
  { return tfactory.cloneType(src); }

Variable *Env::makeVariable(SourceLocation const &L, StringRef n,
                            Type *t, DeclFlags f)
  { return tfactory.makeVariable(L, n, t, f); }

Variable *Env::cloneVariable(Variable *src)
  { return tfactory.cloneVariable(src); }
#endif // 0
