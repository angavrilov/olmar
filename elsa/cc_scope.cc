// cc_scope.cc            see license.txt for copyright and terms of use
// code for cc_scope.h

#include "cc_scope.h"     // this module
#include "trace.h"        // trace
#include "variable.h"     // Variable
#include "cc_type.h"      // CompoundType
#include "cc_env.h"       // doh.  Env::error

Scope::Scope(ScopeKind sk, int cc, SourceLoc initLoc)
  : variables(),
    compounds(),
    enums(),
    changeCount(cc),
    canAcceptNames(true),
    parentScope(),
    scopeKind(sk),
    namespaceVar(NULL),
    usingEdges(0),            // the arrays start as NULL b/c in the
    activeUsingEdges(0),      // common case the sets are empty
    outstandingActiveEdges(0),
    curCompound(NULL),
    curFunction(NULL),
    curTemplateParams(NULL),
    curLoc(initLoc)
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


// This function should not modify 'v'; any changes to 'v' should
// instead be done by 'registerVariable'.
bool Scope::addVariable(Variable *v, bool forceReplace)
{
  xassert(canAcceptNames);

  if (!v->isNamespace()) {
    // classify the variable for debugging purposes
    char const *classification =
      v->hasFlag(DF_TYPEDEF)?    "typedef" :
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
      //v->access = curAccess;      // moved into registerVariable()
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
  }

  // moved into registerVariable()
  //if (isGlobalScope()) {
  //  v->setFlag(DF_GLOBAL);
  //}

  if (insertUnique(variables, v->name, v, changeCount, forceReplace)) {
    afterAddVariable(v);
    return true;
  }
  else {
    return false;
  }
}

void Scope::afterAddVariable(Variable *v)
{}


bool Scope::addCompound(CompoundType *ct)
{
  xassert(canAcceptNames);

  trace("env") << "added " << toString(ct->keyword) << " " << ct->name << endl;

  // set up ct's parent scope if appropriate
  if (getTypedefName()) {
    ct->parentScope = this;
  }

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


// This function is responsble for modifying 'v' in preparation for
// adding it to this scope.  It's separated out from 'addVariable'
// because in certain cases the variable is actually not added; but
// this function should behave as if it always is added.
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

  if (curCompound) {
    if (curCompound->name) {
      // since the scope has a name, let the variable point at it
      v->scope = this;
    }
  
    // take access control from scope's current setting
    v->access = curAccess;
  }

  if (scopeKind == SK_NAMESPACE) {
    // point at enclosing namespace (Q: even for anon namespaces?)
    v->scope = this;
  }

  if (isGlobalScope()) {
    v->setFlag(DF_GLOBAL);
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

  if ((flags & LF_ONLY_NAMESPACES) &&
      !v->hasFlag(DF_NAMESPACE)) {
    return NULL;
  }

  if ((flags & LF_TYPES_NAMESPACES) &&
      !v->hasFlag(DF_TYPEDEF) &&
      !v->hasFlag(DF_NAMESPACE)) {
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
//      cout << "lookupPQVariableC variables" << endl;
//      for (StringSObjDict<Variable>::IterC iter(variables); !iter.isDone(); iter.next()) {
//        cout << "\t" << iter.key() << "=" << iter.value() << endl;
//      }
//      cout << "name->getName() " << name->getName() << endl;
    v1 = vfilterC(variables.queryif(name->getName()), flags);
    
    // 7.3.4 para 1: "active using" edges are a source of additional
    // declarations that can satisfy an unqualified lookup
    if (activeUsingEdges.isNotEmpty()) {
      v1 = searchUsingEdges(name->getName(), env, flags, v1);
    }

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


Variable const *Scope::getTypedefNameC() const
{
  if (scopeKind == SK_CLASS) {
    return curCompound->typedefVar;
  }
  if (scopeKind == SK_NAMESPACE) {
    return namespaceVar;
  }
  return NULL;
}


bool Scope::encloses(Scope const *s) const
{
  // in all situations where this is relevant, 's' has named parent
  // scopes (since we found it by qualified lookup), so just walk
  // the scope parent links

  Scope const *me = this;
  if (me->isGlobalScope()) {
    me = NULL;     // null 'parentScope' means it's in global
  }

  while (s && s!=me) {
    s = s->parentScope;
  }

  return s == me;
}


string Scope::desc() const
{
  Variable const *v = getTypedefNameC();
  if (v) {
    return stringc << toString(scopeKind) << " " << v->name;
  }
  else if (isGlobalScope()) {
    return "global scope";
  }
  else {
    return stringc << "anonymous " << toString(scopeKind) 
                   << " scope at " << stringf("%p", this);
  }
}


void Scope::addUsingEdge(Scope *target)
{
  TRACE("using", "added using-edge from " << desc()
              << " to " << target->desc());

  usingEdges.push(target);
}


void Scope::addActiveUsingEdge(Scope *target)
{
  TRACE("using", "added active-using-edge from " << desc()
              << " to " << target->desc());

  activeUsingEdges.push(target);
}

void Scope::removeActiveUsingEdge(Scope *target)
{
  TRACE("using", "removing active-using-edge from " << desc()
              << " to " << target->desc());

  if (activeUsingEdges.top() == target) {
    // we're in luck
    activeUsingEdges.pop();
  }
  else {
    // find the element that has 'target'
    for (int i=0; i<activeUsingEdges.length(); i++) {
      if (activeUsingEdges[i] == target) {
        // found it, swap with the top element
        Scope *top = activeUsingEdges.pop();
        activeUsingEdges[i] = top;
        return;
      }
    }
    
    xfailure("attempt to remove active-using edge not in the set");
  }
}


void Scope::scheduleActiveUsingEdge(Env &env, Scope *target)
{
  // find the innermost scope that contains both 'this' and 'target',
  // as this is effectively where target's declarations appear while
  // we're in this scope
  Scope *enclosing = env.findEnclosingScope(target);
  enclosing->addActiveUsingEdge(target);

  // schedule it for removal later
  outstandingActiveEdges.push(ActiveEdgeRecord(enclosing, target));
}


void Scope::openedScope(Env &env)
{          
  // all "using" edges give rise to "active using" edges
  for (int i=0; i<usingEdges.length(); i++) {
    scheduleActiveUsingEdge(env, usingEdges[i]);
  }
}

void Scope::closedScope()
{
  // remove the "active using" edges I previously added
  while (outstandingActiveEdges.isNotEmpty()) {
    ActiveEdgeRecord rec = outstandingActiveEdges.pop();

    // hmm.. I notice that I could just have computed 'source'
    // again like I did above..

    rec.source->removeActiveUsingEdge(rec.target);
  }
}

                        
// search the lists instead of maintaining state in the Scope
// objects, to keep things simple; if the profiler tells me
// to make this faster, I can put colors into the objects
static void pushIfWhite(ArrayStack<Scope const*> const &black,
                        ArrayStack<Scope const*> &gray,
                        Scope const *s)
{
  // in the black list?
  int i;
  for (i=0; i<black.length(); i++) {
    if (black[i] == s) {
      return;
    }
  }

  // in the gray list?
  for (i=0; i<gray.length(); i++) {
    if (gray[i] == s) {
      return;
    }
  }

  // not in either, color is effectively white, so push it
  gray.push(s);
}

// DFS over the network of using-directive edges
Variable const *Scope::searchUsingEdges
  (StringRef name, Env &env, LookupFlags flags, Variable const *vfound) const
{
  // set of scopes already searched
  ArrayStack<Scope const*> black;

  // stack of scopes remaining to be searched in the DFS
  ArrayStack<Scope const*> gray;
  
  // the caller already searched among my variables
  black.push(this);

  // we follow the "active using" edges from 'this' to initialize
  // the stack; after that, we'll use the "using" edges
  for (int i=0; i<activeUsingEdges.length(); i++) {
    pushIfWhite(black, gray, activeUsingEdges[i]);
  }

  // process the gray set until empty
  while (gray.isNotEmpty()) {
    Scope const *s = gray.pop();

    // look for 'name' in 's'
    Variable const *v = vfilterC(s->variables.queryif(name), flags);
    if (v) {
      if (vfound) {
        env.error(stringc
          << "ambiguous lookup: `" << vfound->fullyQualifiedName()
          << "' vs `" << v->fullyQualifiedName() << "'");
      }
      else {
        vfound = v;
      }
    }

    // done with 's'
    black.push(s);

    // push the "using" edges' targets
    for (int i=0; i < s->usingEdges.length(); i++) {
      pushIfWhite(black, gray, usingEdges[i]);
    }
  }

  return vfound;
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

// FIX: Would be cleaner to implement this as a call to
// PQ_fullyQualifiedName() below and then to a toString() method.
// FIX: This is wrong as it does not take into account template
// arguments; Should be moved into CompoundType and done right.
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

  Variable *v = getTypedefName();
  xassert(v);
  sb << "::" << v->name;
  return sb;
}
