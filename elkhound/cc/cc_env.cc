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
Env::Env(StringTable &s, CCLang &L)
  : scopes(),
    disambiguateOnly(false),
    anonTypeCounter(1),
    errors(),
    str(s),
    lang(L),                      
    
    // filled in below; initialized for definiteness
    type_info_const_ref(NULL),
    conversionOperatorName(NULL),
    dependentTypeVar(NULL)
{
  // create first scope
  SourceLocation emptyLoc;
  scopes.prepend(new Scope(0 /*changeCount*/, emptyLoc));

  // create the typeid type
  CompoundType *ct = new CompoundType(CompoundType::K_CLASS, str("type_info"));
  // TODO: put this into the 'std' namespace
  // TODO: fill in the proper fields and methods
  type_info_const_ref = makeRefType(makeCVType(ct, CV_CONST));

  // some special names; pre-computed (instead of just asking the
  // string table for it each time) because in certain situations
  // I compare against them frequently; the main criteria for
  // choosing these names is they have to be untypable by the C++
  // programmer (i.e. if the programmer types them they won't be
  // lexed as single names)
  conversionOperatorName = str("conversion-operator");
  constructorSpecialName = str("constructor-special");

  dependentTypeVar = new Variable(emptyLoc, str("<dependentTypeVar>"),
                                  getSimpleType(ST_DEPENDENT), DF_TYPEDEF);

  // create declarations for some built-in operators

  // void operator delete (void *p);
  {                                                                  
    FunctionType *ft = new FunctionType(getSimpleType(ST_VOID), CV_NONE);
    SourceLocation loc;
    Variable *p = new Variable(loc, str("p"), makePtrType(getSimpleType(ST_VOID)), DF_NONE);
    ft->addParam(new Parameter(p->name, p->type, p));
    Variable *del = new Variable(loc, str("operator delete"), ft, DF_NONE);
    addVariable(del);
  }

  // void operator delete[] (void *p);
  {
    FunctionType *ft = new FunctionType(getSimpleType(ST_VOID), CV_NONE);
    SourceLocation loc;
    Variable *p = new Variable(loc, str("p"), makePtrType(getSimpleType(ST_VOID)), DF_NONE);
    ft->addParam(new Parameter(p->name, p->type, p));
    Variable *del = new Variable(loc, str("operator delete[]"), ft, DF_NONE);
    addVariable(del);
  }
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
    if (!s->canAcceptNames) continue;     // skip template scopes
    if (s->curCompound) continue;         // skip class scopes
    
    return s;
  }

  xfailure("couldn't find the outer scope!");
  return NULL;    // silence warning
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
  bool dummy;
  return lookupQualifiedScope(name, dummy);
}

Scope *Env::lookupQualifiedScope(PQName const *name, bool &dependent)
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
      // check for a special case: a qualifier that refers to
      // a template parameter
      {
        Variable *qualVar =
          scope==NULL? lookupVariable(qual, false /*innerOnly*/) :
                       scope->lookupVariable(qual, false /*innerOnly*/, *this);
        if (qualVar &&
            qualVar->hasFlag(DF_TYPEDEF) &&
            qualVar->type->isTypeVariable()) {
          // we're looking inside an uninstantiated template parameter
          // type; the lookup fails, but no error is generated here
          dependent = true;     // tell the caller what happened
          return NULL;
        }
      }

      // look for a class called 'qual' in scope-so-far; since the
      CompoundType *ct =
        scope==NULL? lookupCompound(qual, false /*innerOnly*/) :
                     scope->lookupCompound(qual, false /*innerOnly*/);
      if (!ct) {
        // I'd like to include some information about which scope
        // we were looking in, but I don't want to be computing
        // intermediate scope names for successful lookups; also,
        // I am still considering adding some kind of scope->name()
        // functionality, which would make this trivial.
        //
        // alternatively, I could just re-traverse the original name;
        // I'm lazy for now
        error(stringc
          << "cannot find class `" << qual << "' for `" << *name << "'",
          true /*disambiguating*/);
        return NULL;
      }

      // check template argument compatibility
      if (!!qualifier->targs != ct->isTemplate()) {
        if (qualifier->targs) {
          error(stringc
            << "class `" << qual << "' isn't a template");
        }
        else {
          error(stringc
            << "class `" << qual
            << "' is a template, you have to supply template arguments");
        } 
        
        // actually, let the typechecker use the scope even without
        // template arguments (for now?)
        //return NULL;
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


Variable *Env::lookupPQVariable(PQName const *name)
{
  Variable *var;

  if (name->hasQualifiers()) {
    // look up the scope named by the qualifiers
    bool dependent = false;
    Scope *scope = lookupQualifiedScope(name, dependent);
    if (!scope) {
      if (dependent) {
        // tried to look into a template parameter
        return dependentTypeVar;
      }
      else {
        // error has already been reported
        return NULL;
      }
    }

    // look inside the final scope for the final name
    var = scope->lookupVariable(name->getName(), false /*innerOnly*/, *this);
    if (!var) {
      error(stringc
        << name->qualifierString() << " has no member called `"
        << name->getName() << "'",
        true /*disambiguating*/);
      return NULL;
    }
  }

  else {
    var = lookupVariable(name->getName(), false /*innerOnly*/);
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

Variable *Env::lookupVariable(StringRef name, bool innerOnly)
{
  if (innerOnly) {
    // here as in every other place 'innerOnly' is true, I have
    // to skip non-accepting scopes since that's not where the
    // client is planning to put the name
    return acceptingScope()->lookupVariable(name, innerOnly, *this);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    Variable *v = iter.data()->lookupVariable(name, innerOnly, *this);
    if (v) {
      return v;
    }
  }
  return NULL;    // not found
}

CompoundType *Env::lookupPQCompound(PQName const *name)
{   
  // same logic as for lookupPQVariable
  if (name->hasQualifiers()) {
    Scope *scope = lookupQualifiedScope(name);
    if (!scope) return NULL;

    CompoundType *ret = scope->lookupCompound(name->getName(), false /*innerOnly*/);
    if (!ret) {
      error(stringc
        << name->qualifierString() << " has no class/struct/union called `"
        << name->getName() << "'",
        true /*disambiguating*/);
      return NULL;
    }

    return ret;
  }

  return lookupCompound(name->getName(), false /*innerOnly*/);
}

CompoundType *Env::lookupCompound(StringRef name, bool innerOnly)
{
  if (innerOnly) {
    return acceptingScope()->lookupCompound(name, innerOnly);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    CompoundType *ct = iter.data()->lookupCompound(name, innerOnly);
    if (ct) {
      return ct;
    }
  }
  return NULL;    // not found
}

EnumType *Env::lookupPQEnum(PQName const *name)
{
  // same logic as for lookupPQVariable
  if (name->hasQualifiers()) {
    Scope *scope = lookupQualifiedScope(name);
    if (!scope) return NULL;

    EnumType *ret = scope->lookupEnum(name->getName(), false /*innerOnly*/);
    if (!ret) {
      error(stringc
        << name->qualifierString() << " has no enum called `"
        << name->getName() << "'",
        true /*disambiguating*/);
      return NULL;
    }

    return ret;
  }

  return lookupEnum(name->getName(), false /*innerOnly*/);
}

EnumType *Env::lookupEnum(StringRef name, bool innerOnly)
{
  if (innerOnly) {
    return acceptingScope()->lookupEnum(name, innerOnly);
  }

  // look in all the scopes
  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    EnumType *et = iter.data()->lookupEnum(name, false /*innerOnly*/);
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
  CompoundType const *tclass, FakeList<TemplateArgument> *args)
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
  ObjListIter<Parameter> paramIter(tclass->templateInfo->params);

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
    Parameter const *param = paramIter.data();

    if (!param->type->isTypeVariable()) {
      env.unimp("non-class template parameters");
      goto cleanup;
    }

    // make a variable that typedef's the argument type to be
    // the parameter name
    Variable *argVar
      = new Variable(param->decl->loc, param->name,
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
  ret->syntax->tcheck(env);

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
    = new Variable(tclass->typedefVar->loc, instNameRef,
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
  if (lookupVariable(sr, false /*innerOnly*/)) {
    return getAnonName(keyword);    // try again
  }
  else {
    return sr;
  }
}


// -------- diagnostics --------
Type const *Env::error(char const *msg, bool disambiguates)
{
  trace("error") << (disambiguates? "[d] " : "") << "error: " << msg << endl;
  if (!disambiguateOnly || disambiguates) {
    errors.prepend(new ErrorMsg(
      stringc << "error: " << msg, false /*isWarning*/, loc(), disambiguates));
  }
  return getSimpleType(ST_ERROR);
}


Type const *Env::warning(char const *msg)
{
  trace("error") << "warning: " << msg << endl;
  if (!disambiguateOnly) {
    errors.prepend(new ErrorMsg(
      stringc << "warning: " << msg, true /*isWarning*/, loc(), false /*disambiguates*/));
  }
  return getSimpleType(ST_ERROR);
}


Type const *Env::unimp(char const *msg)
{
  // always print this immediately, because in some cases I will
  // segfault (deref'ing NULL) right after printing this
  cout << "unimplemented: " << msg << endl;

  errors.prepend(new ErrorMsg(
    stringc << "unimplemented: " << msg, false /*isWarning*/, loc(), false /*disambiguates*/));
  return getSimpleType(ST_ERROR);
}


Type const *Env::error(Type const *t, char const *msg)
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


