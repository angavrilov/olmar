// cc_scope.cc            see license.txt for copyright and terms of use
// code for cc_scope.h

#include "cc_scope.h"     // this module
#include "trace.h"        // trace
#include "variable.h"     // Variable
#include "cc_type.h"      // CompoundType
#include "cc_env.h"       // doh.  Env::error

Scope::Scope(int cc, SourceLoc initLoc)
  : variables(),
    compounds(),
    enums(),
    changeCount(cc),
    canAcceptNames(true),
    innerClasses(),
    parentScope(),
    isParameterListScope(false),
    curCompound(NULL),
    curFunction(NULL),
    curTemplateParams(NULL),
    curLoc(initLoc)
{}

Scope::~Scope()
{
  if (curTemplateParams) {
    // this happens when I open a template-param scope for
    // a template class member function; the member function
    // doesn't need the params (since the class carries them)
    // so they stay here until deleted
    delete curTemplateParams;
  }
}


// -------- insertion --------
template <class T>
bool insertUnique(StringSObjDict<T> &table, char const *key, T *value,
                  int &changeCount, bool forceReplace)
{
  if (table.isMapped(key)) {
    if (!forceReplace) {
      return false;
    }
    else {
      // remove the old mapping
      table.remove(key);
    }
  }

  table.add(key, value);
  changeCount++;
  return true;
}


bool Scope::addVariable(Variable *v, bool forceReplace)
{
  xassert(canAcceptNames);

  // classify the variable for debugging purposes
  char const *classification =
    v->hasFlag(DF_TYPEDEF)? "typedef" :
    v->type->isFunctionType()? "function" :
                               "variable" ;

  // does the type contain any error-recovery types?  if so,
  // then we don't want to add the variable because we could
  // be trying to disambiguate a declarator, and this would
  // be the erroneous interpretation which is not supposed to
  // modify the environment
  bool containsErrors = v->type->containsErrors();
  char const *prefix = "";
  if (containsErrors) {
    prefix = "[cancel/error] ";
  }

  if (!curCompound) {
    // variable outside a class
    trace("env") << prefix << "added " << classification
                 << " `" << v->name
                 << "' of type `" << v->type->toString()
                 << "' at " << toString(v->loc)
                 << endl;
  }
  else {
    // class member
    v->access = curAccess;
    trace("env") << prefix << "added " << toString(v->access)
                 << " member " << classification
                 << " `" << v->name
                 << "' of type `" << v->type->toString()
                 << "' at " << toString(v->loc)
                 << " to " << curCompound->keywordAndName()
                 << endl;
  }

  if (containsErrors) {
    return true;     // pretend it worked; don't further rock the boat
  }

  return insertUnique(variables, v->name, v, changeCount, forceReplace);
}


bool Scope::addCompound(CompoundType *ct)
{
  xassert(canAcceptNames);

  trace("env") << "added " << toString(ct->keyword) << " " << ct->name << endl;

  ct->access = curAccess;
  if (insertUnique(compounds, ct->name, ct, changeCount, false /*forceReplace*/)) {
    // WORKAROUND: add it to a list also..
    innerClasses.append(ct);
    return true;
  }             
  else {
    return false;
  }
}


bool Scope::addEnum(EnumType *et)
{
  xassert(canAcceptNames);

  trace("env") << "added enum " << et->name << endl;

  et->access = curAccess;
  return insertUnique(enums, et->name, et, changeCount, false /*forceReplace*/);
}


void Scope::registerVariable(Variable *v)
{
  if (curCompound && curCompound->name) {
    // since the scope has a name, let the variable point at it
    v->scope = this;
  }
}


// -------- lookup --------                            
// vfilter: variable filter
// implements variable-filtering aspect of the flags; the idea
// is you never query 'variables' without wrapping the call
// in a filter
Variable const *vfilterC(Variable const *v, LookupFlags flags)
{
  if (!v) return v;

  if ((flags & LF_ONLY_TYPES) &&
      !v->hasFlag(DF_TYPEDEF)) {
    return NULL;
  }

  return v;
}

Variable *vfilter(Variable *v, LookupFlags flags)
  { return const_cast<Variable*>(vfilterC(v, flags)); }


// this sets 'crossVirtual' to true if we had to cross a
// virtual inheritance boundary to find the name; otherwise
// it leaves 'crossVirtual' unchanged
Variable const *Scope
  ::lookupPQVariableC(PQName const *name, bool &crossVirtual,
                      Env &env, LookupFlags flags) const
{
  Variable const *v1 = NULL;

  // [cppstd sec. 10.2]: class members hide all members from
  // base classes
  if (!name->hasQualifiers()) {
    v1 = vfilterC(variables.queryif(name->getName()), flags);
    if (v1) {
      return v1;
    }
  }

  if (!curCompound) {
    // nowhere else to look
    return NULL;
  }

  // [ibid] if I can find a class member by looking in multiple
  // base classes, then it's ambiguous and the program has
  // an error
  //
  // to implement this, I'll walk the list of base classes,
  // keeping in 'ret' the latest successful lookup, and if I
  // find another successful lookup then I'll complain
  CompoundType const *v1Base = NULL;
  bool v1CrossVirtual = false;
  FOREACH_OBJLIST(BaseClass, curCompound->bases, iter) {
    CompoundType const *v2Base = iter.data()->ct;
    bool v2CrossVirtual = iter.data()->isVirtual;

    // if we're looking for a qualified name, and the outermost
    // qualifier matches this base class, then consume it
    PQName const *innerName = name;
    if (name->hasQualifiers() &&
        name->asPQ_qualifierC()->qualifier == v2Base->name) {
      innerName = name->asPQ_qualifierC()->rest;
    }

    Variable const *v2 =
      v2Base->lookupPQVariableC(innerName, v2CrossVirtual, env, flags);
    if (!v2) {
      continue;
    }

    trace("lookup") << "found " << v2Base->name << "::" << name
                    << ", crossed virtual: " << v2CrossVirtual << endl;

    if (v1) {
      if (v1 == v2 && v1->hasFlag(DF_STATIC)) {
        // they both refer to the same static entity; that's ok
        continue;
      }

      if (v1 == v2 && v1CrossVirtual && v2CrossVirtual) {
        // they both refer to the same entity and both inheritance
        // paths cross a virtual inheritance edge; ok
        // (I hope this analysis is sound.. it's kind of hard to
        // reason about..)
        continue;
      }

      // ambiguity
      env.error(stringc
        << "reference to `" << *name << "' is ambiguous, because "
        << "it could either refer to "
        << v1Base->name << "::" << *name << " or "
        << v2Base->name << "::" << *name);
      continue;
    }

    // found one; copy it into 'v1', my "best so far"
    v1 = v2;
    v1Base = v2Base;
    v1CrossVirtual = v2CrossVirtual;
  }

  // if we found something, tell the caller whether we crossed
  // a virtual inheritance boundary
  if (v1) {
    crossVirtual = v1CrossVirtual;
  }

  return v1;
}


Variable const *Scope
  ::lookupPQVariableC(PQName const *name, Env &env, LookupFlags flags) const
{
  bool dummy;
  return lookupPQVariableC(name, dummy, env, flags);
}


Variable const *Scope
  ::lookupVariableC(StringRef name, Env &env, LookupFlags flags) const
{
  if (flags & LF_INNER_ONLY) {
    return vfilterC(variables.queryif(name), flags);
  }

  PQ_name wrapperName(name);
  Variable const *ret = lookupPQVariableC(&wrapperName, env, flags);
  if (ret) return ret;

  // no qualifiers and not found yet; look in parent scope
  // [cppstd 3.4.1 para 8]
  if (parentScope) {
    return parentScope->lookupVariableC(name, env, flags);
  }
  else {
    return NULL;
  }
}


CompoundType const *Scope::lookupCompoundC(StringRef name, LookupFlags /*flags*/) const
{
  // TODO: implement base class lookup for CompoundTypes
  // (rather obscure, since most uses of types defined in the
  // base classes will be without the elaborated type specifier
  // (e.g. "class"), so the typedefs will be used and that's
  // covered by the above case for Variables)

  return compounds.queryif(name);
}

EnumType const *Scope::lookupEnumC(StringRef name, LookupFlags /*flags*/) const
{
  // TODO: implement base class lookup for EnumTypes

  return enums.queryif(name);
}


