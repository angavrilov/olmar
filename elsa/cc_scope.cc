// cc_scope.cc            see license.txt for copyright and terms of use
// code for cc_scope.h

#include "cc_scope.h"     // this module
#include "trace.h"        // trace
#include "variable.h"     // Variable
#include "cc_type.h"      // CompoundType
#include "cc_env.h"       // doh.  Env::error

Scope::Scope(ScopeKind sk, int cc, SourceLoc initLoc, TranslationUnit *tunit0)
  : variables(),
    compounds(),
    enums(),
    changeCount(cc),
    canAcceptNames(true),
    parentScope(),
    scopeKind(sk),
    curCompound(NULL),
    curFunction(NULL),
    curTemplateParams(NULL),
    curLoc(initLoc),
    tunit(tunit0)
{
  xassert(sk != SK_UNKNOWN);
}

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
                 << " (" << toString(v->scopeKind) << " scope)"
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

  if (isGlobalScope()) {
    v->setFlag(DF_GLOBAL);
  }

  // if is a data member, not a method, static data, or a typedef
  if (!v->type->isFunctionType() && !v->hasFlag(DF_STATIC) && !v->hasFlag(DF_TYPEDEF)) {
    // FIX: Do I want this here as well?
//      ! (v0->hasFlag(DF_ENUMERATOR)
    // FIX: Don't know how to avoid making an int on the heap, as that
    // is the way that the templatized class StringSObjDict is set up.
    name_pos.add(v->name, new int(data_variables_in_order.count()));// garbage? is it ever deleted?
    data_variables_in_order.append(v);
  }
  return insertUnique(variables, v->name, v, changeCount, forceReplace);
}


bool Scope::addCompound(CompoundType *ct)
{
  xassert(canAcceptNames);

  trace("env") << "added " << toString(ct->keyword) << " " << ct->name << endl;

  ct->access = curAccess;
  return insertUnique(compounds, ct->name, ct, changeCount, false /*forceReplace*/);
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
  if (v->scopeKind == SK_UNKNOWN) {
    v->scopeKind = scopeKind;
  }
  else {
    // this happens to parameters at function definitions: the
    // variable is created and first entered into the parameter scope,
    // but then the they are also inserted into the function scope
    // (because the parameter scope ends at the closing-paren); so
    // leave the scope kind alone, even though now the variable has
    // been added to a function scope
  }

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


// the lookup semantics here are a bit complicated; relevant tests
// include t0099.cc and std/3.4.5.cc
Variable const *Scope
  ::lookupPQVariableC(PQName const *name, Env &env, LookupFlags flags) const
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

  // recursively examine the subobject hierarchy
  CompoundType const *v1Base = NULL;
  curCompound->clearSubobjVisited();
  lookupPQVariableC_considerBase(name, env, flags,
                                 v1, v1Base, &curCompound->subobj);

  return v1;
}

// helper for lookupPQVariableC; if we find a suitable variable, set
// v1/v1Base to refer to it; if we find an ambiguity, report that
void Scope::lookupPQVariableC_considerBase
  (PQName const *name, Env &env, LookupFlags flags,  // stuff from caller
   Variable const *&v1,                              // best so far
   CompoundType const *&v1Base,                      // where 'v1' was found
   BaseClassSubobj const *v2Subobj) const            // where we're looking now 
{
  if (v2Subobj->visited) return;
  v2Subobj->visited = true;

  CompoundType const *v2Base = v2Subobj->ct;

  // if we're looking for a qualified name, and the outermost
  // qualifier matches this base class, then consume it
  if (name->hasQualifiers() &&
      name->asPQ_qualifierC()->qualifier == v2Base->name) {
    name = name->asPQ_qualifierC()->rest;
  }

  if (!name->hasQualifiers()) {
    // look in 'v2Base' for the field
    Variable const *v2 =
      vfilterC(v2Base->variables.queryif(name->getName()), flags);
    if (v2) {
      trace("lookup") << "found " << v2Base->name << "::"
                      << name->toString() << endl;

      if (v1) {
        if (v1 == v2 && v1->hasFlag(DF_STATIC)) {
          // they both refer to the same static entity; that's ok
          return;
        }

        // ambiguity
        env.error(stringc
          << "reference to `" << *name << "' is ambiguous, because "
          << "it could either refer to "
          << v1Base->name << "::" << *name << " or "
          << v2Base->name << "::" << *name);
        return;
      }

      // found one; copy it into 'v1', my "best so far"
      v1 = v2;
      v1Base = v2Base;
      
      // this name hides any in base classes, so this arm of the
      // search stops here
      return;
    }
  }

  // name is still qualified, or we didn't find it; recursively
  // look into base classes of 'v2Subobj'
  SFOREACH_OBJLIST(BaseClassSubobj, v2Subobj->parents, iter) {
    lookupPQVariableC_considerBase(name, env, flags, v1, v1Base, iter.data());
  }
}


Variable const *Scope
  ::lookupVariableC(StringRef name, Env &env, LookupFlags flags) const
{
  if (flags & LF_INNER_ONLY) {
    return vfilterC(variables.queryif(name), flags);
  }

  PQ_name wrapperName(SL_UNKNOWN, name);
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


// true if this scope is a member of the global scope
bool Scope::immediateGlobalScopeChild() 
{
  return curCompound && curCompound->typedefVar->hasFlag(DF_GLOBAL);
}

// are we in a scope where the parent chain terminates in a global?
// That is, can fullyQualifiedName() be called on this scope without
// failing?
bool Scope::linkerVisible() 
{
  if (parentScope) {
    return parentScope->linkerVisible();
  }
  else {
    return immediateGlobalScopeChild();
  }
}

string Scope::fullyQualifiedName() 
{
  stringBuilder sb;
  if (parentScope) {
    sb = parentScope->fullyQualifiedName();
  }
  else {
    if (!immediateGlobalScopeChild()) {
      // we didn't end up in the global scope; for example a function
      // in a class in a function
      xfailure("fullyQualifiedName called on scope that doesn't terminate in the global scope");
    }
  }          

  xassert(curCompound);
  sb << "::" << curCompound->name;
  return sb;
}
