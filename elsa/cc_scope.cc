// cc_scope.cc            see license.txt for copyright and terms of use
// code for cc_scope.h

#include "cc_scope.h"     // this module
#include "trace.h"        // trace
#include "variable.h"     // Variable
#include "cc_type.h"      // CompoundType
#include "cc_env.h"       // doh.  Env::error

Scope::Scope(int cc, SourceLocation const &initLoc)
  : variables(),
    compounds(),
    enums(),
    changeCount(cc),
    canAcceptNames(true),
    curCompound(NULL),
    curFunction(NULL),
    templateParams(NULL),
    curLoc(initLoc)
{}

Scope::~Scope()
{
  if (templateParams) {
    // this happens when I open a template-param scope for
    // a template class member function; the member function
    // doesn't need the params (since the class carries them)
    // so they stay here until deleted
    delete templateParams;
  }
}


// -------- insertion --------
template <class T>
bool insertUnique(StringSObjDict<T> &table, char const *key, T *value,
                  int &changeCount)
{
  if (table.isMapped(key)) {
    return false;
  }
  else {
    table.add(key, value);
    changeCount++;
    return true;
  }
}


bool Scope::addVariable(Variable *v)
{
  xassert(canAcceptNames);

  // classify the variable for debugging purposes
  char const *classification =
    v->hasFlag(DF_TYPEDEF)? "typedef" :
    v->type->isFunctionType()? "function" :
                               "variable" ;

  if (!curCompound) {
    // variable outside a class
    trace("env") << "added " << classification 
                 << " `" << v->name
                 << "' of type `" << v->type->toString()
                 << "' at " << v->loc.toString() 
                 << endl;
  }
  else {
    // class member
    v->access = curAccess;
    trace("env") << "added " << toString(v->access)
                 << " member " << classification
                 << " `" << v->name
                 << "' of type `" << v->type->toString()
                 << "' at " << v->loc.toString()
                 << " to " << curCompound->keywordAndName()
                 << endl;
  }

  return insertUnique(variables, v->name, v, changeCount);
}


bool Scope::addCompound(CompoundType *ct)
{
  xassert(canAcceptNames);

  trace("env") << "added " << toString(ct->keyword) << " " << ct->name << endl;

  ct->access = curAccess;
  return insertUnique(compounds, ct->name, ct, changeCount);
}


bool Scope::addEnum(EnumType *et)
{
  xassert(canAcceptNames);

  trace("env") << "added enum " << et->name << endl;

  et->access = curAccess;
  return insertUnique(enums, et->name, et, changeCount);
}


// -------- lookup --------
// this sets 'crossVirtual' to true if we had to cross a
// virtual inheritance boundary to find the name; otherwise
// it leaves 'crossVirtual' unchanged
Variable const *Scope
  ::lookupVariableC(StringRef name, bool &crossVirtual, 
                    bool innerOnly, Env &env) const
{
  // [cppstd sec. 10.2]: class members hide all members from
  // base classes
  Variable const *v1 = variables.queryif(name);
  if (v1 || innerOnly) {
    return v1;
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
    Variable const *v2 = 
      v2Base->lookupVariableC(name, v2CrossVirtual, innerOnly, env);

    if (v2) {
      trace("lookup") << "found " << v2Base->name << "::" << name 
                      << ", crossed virtual: " << v2CrossVirtual << endl;
    }

    if (v1 && v2) {
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
        << "reference to `" << name << "' is ambiguous, because "
        << "it could either refer to "
        << v1Base->name << "::" << name << " or "
        << v2Base->name << "::" << name);
      continue;
    }

    if (v2) {
      // found one; copy it into 'v1', my "best so far"
      v1 = v2;
      v1Base = v2Base;
      v1CrossVirtual = v2CrossVirtual;
    }
  }

  // if we found something, tell the caller whether we crossed
  // a virtual inheritance boundary
  if (v1) {
    crossVirtual = v1CrossVirtual;
  }

  return v1;
}


Variable const *Scope
  ::lookupVariableC(StringRef name, bool innerOnly, Env &env) const
{
  bool dummy;
  return lookupVariableC(name, dummy, innerOnly, env);
}


CompoundType const *Scope::lookupCompoundC(StringRef name, bool /*innerOnly*/) const
{
  // TODO: implement base class lookup for CompoundTypes
  // (rather obscure, since most uses of types defined in the
  // base classes will be without the elaborated type specifier
  // (e.g. "class"), so the typedefs will be used and that's
  // covered by the above case for Variables)

  return compounds.queryif(name);
}

EnumType const *Scope::lookupEnumC(StringRef name, bool /*innerOnly*/) const
{
  // TODO: implement base class lookup for EnumTypes

  return enums.queryif(name);
}
