// cc_scope.h            see license.txt for copyright and terms of use
// a C++ scope, which is used by the Env parsing environment
// and also by CompoundType to store members

#ifndef CC_SCOPE_H
#define CC_SCOPE_H

#include "strsobjdict.h"  // StrSObjDict
#include "cc_flags.h"     // AccessKeyword
#include "srcloc.h"       // SourceLoc
#include "strtable.h"     // StringRef
#include "sobjlist.h"     // SObjList
#include "array.h"        // ArrayStack

// NOTE: We cannot #include cc_type.h b/c cc_type.h #includes cc_scope.h.

class Env;                // cc_env.h
class Variable;           // variable.h
class CompoundType;       // cc_type.h
class BaseClassSubobj;    // cc_type.h
class EnumType;           // cc_type.h
class Function;           // cc.ast
class TemplateParams;     // cc_type.h
class PQName;             // cc.ast
class TranslationUnit;    // cc.ast.gen.h


// variable lookup sometimes has complicated exceptions or
// special cases, so I'm folding lookup options into one value
enum LookupFlags {
  LF_NONE            = 0,
  LF_INNER_ONLY      = 0x01,    // only look in the innermost scope
  LF_ONLY_TYPES      = 0x02,    // ignore (skip over) non-type names
  LF_TYPENAME        = 0x04,    // user used 'typename' keyword
  LF_SKIP_CLASSES    = 0x08,    // skip class scopes
  LF_ONLY_NAMESPACES = 0x10,    // ignore non-namespace names
  LF_TYPES_NAMESPACES= 0x20,    // ignore non-type, non-namespace names
  LF_QUALIFIED       = 0x40,    // context is a qualified lookup

  LF_ALL_FLAGS       = 0xFF,    // bitwise OR of all flags
};

// experiment: will this work?  yes
inline LookupFlags operator| (LookupFlags f1, LookupFlags f2)
  { return (LookupFlags)((int)f1 | (int)f2); }
inline LookupFlags operator& (LookupFlags f1, LookupFlags f2)
  { return (LookupFlags)((int)f1 & (int)f2); }
inline LookupFlags operator~ (LookupFlags f)
  { return (LookupFlags)((~(int)f) & LF_ALL_FLAGS); }


// information about a single scope: the names defined in it,
// any "current" things being built (class, function, etc.)
class Scope {
private:     // types
  // for recording information about "active using" edges that
  // need to be cancelled at scope exit
  class ActiveEdgeRecord {
  public:
    Scope *source, *target;     // edge exists from source to target
    
  public:
    ActiveEdgeRecord()
      : source(NULL), target(NULL) {}
    ActiveEdgeRecord(Scope *s, Scope *t)
      : source(s), target(t) {}
      
    ActiveEdgeRecord& operator= (ActiveEdgeRecord const &obj)
      { source=obj.source; target=obj.target; return *this; }
  };

private:     // data
  // variables: name -> Variable
  // note: this includes typedefs (DF_TYPEDEF is set), and it also
  // includes enumerators (DF_ENUMERATOR is set)
  StringSObjDict<Variable> variables;

  // compounds: map name -> CompoundType
  StringSObjDict<CompoundType> compounds;

  // enums: map name -> EnumType
  StringSObjDict<EnumType> enums;

  // per-scope change count
  int changeCount;

public:      // data
  // when this is set to false, the environment knows it should not
  // put new names into this scope, but rather go further down into
  // the scope stack to insert the name (used for scopes of template
  // parameters, after the names have been added)
  bool canAcceptNames;

  // (serf) This is the parent (enclosing) scope, but only if that
  // scope has a name (rationale: allow anonymous scopes to be
  // deallocated).  For classes, this field is only set to non-NULL
  // after the inner class has been fully constructed, since we can
  // rely on the Environment's scope stack to look up things in
  // containing classes while building the inner class for the first
  // time (why did I do that??).  For namespaces, it's set as soon as
  // the namespace is created.
  Scope *parentScope;

  // what kind of scope is this?
  ScopeKind scopeKind;

  // if this is a namespace, this points to the variable used to
  // find the namespace during lookups
  Variable *namespaceVar;

  // --------------- for using-directives ----------------
  // possible optim:  For most scopes, these three arrays waste 13
  // words of storage.  I could collect them into a separate structure
  // and just keep a pointer here, making it non-NULL only when
  // something gets put into an array.

  // set of "using" edges; these affect lookups transitively when
  // other scopes have active edges to this one
  ArrayStack<Scope*> usingEdges;

  // this is the in-degree of the usingEdges network; it is used to
  // tell when a scope has someone using it, because that means we
  // may need to recompute the active-using edges
  int usingEdgesRefct;

  // set of "active using" edges; these directly influence lookups
  // in this scope
  ArrayStack<Scope*> activeUsingEdges;
  
  // set of "active using" edges in other scopes that need to be
  // retracted once this scope exits
  ArrayStack<ActiveEdgeRecord> outstandingActiveEdges;

  // ------------- "current" entities -------------------
  // these are set to allow the typechecking code to know about
  // the context we're in
  CompoundType *curCompound;          // (serf) CompoundType we're building
  AccessKeyword curAccess;            // access disposition in effect
  Function *curFunction;              // (serf) Function we're analyzing
  TemplateParams *curTemplateParams;  // (owner) params to attach to next function or class
  SourceLoc curLoc;                   // latest AST location marker seen

private:     // funcs
  void Scope::lookupPQVariableC_considerBase
    (PQName const *name, Env &env, LookupFlags flags,
     Variable const *&v1, CompoundType const *&v1Base,
     BaseClassSubobj const *v2Subobj) const;

  // more using-directive stuff
  void addActiveUsingEdge(Scope *target);
  void removeActiveUsingEdge(Scope *target);
  void scheduleActiveUsingEdge(Env &env, Scope *target);
  Variable const *searchActiveUsingEdges
    (StringRef name, Env &env, LookupFlags flags, Variable const *vfound) const;
  Variable const *searchUsingEdges
    (StringRef name, Env &env, LookupFlags flags) const;
  void getUsingClosure(ArrayStack<Scope*> &dest);

protected:   // funcs
  // this function is called at the end of addVariable, after the
  // Variable has been added to the 'variables' map; it's intended
  // that CompoundType will override this, for the purpose of
  // maintaining 'dataMembers'
  virtual void afterAddVariable(Variable *v);

public:      // funcs
  Scope(ScopeKind sk, int changeCount, SourceLoc initLoc);
  virtual ~Scope();     // virtual to silence warning; destructor is not part of virtualized interface

  int getChangeCount() const { return changeCount; }

  // some syntactic sugar on the scope kind
  bool isGlobalScope() const        { return scopeKind == SK_GLOBAL; }
  bool isParameterScope() const     { return scopeKind == SK_PARAMETER; }
  bool isFunctionScope() const      { return scopeKind == SK_FUNCTION; }
  bool isClassScope() const         { return scopeKind == SK_CLASS; }
  bool isTemplateScope() const      { return scopeKind == SK_TEMPLATE; }
  bool isNamespace() const          { return scopeKind == SK_NAMESPACE; }

  // insertion; these return false if the corresponding map already
  // has a binding (unless 'forceReplace' is true)
  bool addVariable(Variable *v, bool forceReplace=false);
  bool addCompound(CompoundType *ct);
  bool addEnum(EnumType *et);

  // mark 'v' as being a member of this scope, by setting its 'scope'
  // and 'scopeKind' members (this is not done by 'addVariable')
  void registerVariable(Variable *v);

  // somewhat common sequence: register, add, assert that the add worked
  void addUniqueVariable(Variable *v);

  // lookup; these return NULL if the name isn't found; 'env' is
  // passed for the purpose of reporting ambiguity errors
  Variable const *lookupVariableC(StringRef name, Env &env, LookupFlags f=LF_NONE) const;
  CompoundType const *lookupCompoundC(StringRef name, LookupFlags f=LF_NONE) const;
  EnumType const *lookupEnumC(StringRef name, LookupFlags f=LF_NONE) const;

  // lookup of a possibly-qualified name; used for member access
  // like "a.B::f()"
  Variable const *lookupPQVariableC(PQName const *name, Env &env, LookupFlags f=LF_NONE) const;

  // non-const versions..
  Variable *lookupVariable(StringRef name, Env &env, LookupFlags f=LF_NONE)
    { return const_cast<Variable*>(lookupVariableC(name, env, f)); }
  CompoundType *lookupCompound(StringRef name, LookupFlags f=LF_NONE)
    { return const_cast<CompoundType*>(lookupCompoundC(name, f)); }
  EnumType *lookupEnum(StringRef name, LookupFlags f=LF_NONE)
    { return const_cast<EnumType*>(lookupEnumC(name, f)); }
  Variable *lookupPQVariable(PQName const *name, Env &env, LookupFlags f=LF_NONE)
    { return const_cast<Variable*>(lookupPQVariableC(name, env, f)); }

  // for iterating over the variables
  StringSObjDict<Variable>::IterC getVariableIter() const
    { return StringSObjDict<Variable>::IterC(variables); }

  // and the inner classes
  StringSObjDict<CompoundType>::IterC getCompoundIter() const
    { return StringSObjDict<CompoundType>::IterC(compounds); }

  // lookup within the 'variables' map, without consulting base
  // classes, etc.; returns NULL if not found
  Variable *rawLookupVariable(char const *name)
    { return variables.queryif(name); }

  int private_compoundTop() const
    { return compounds.private_getTopAddr(); }

  // if this scope has a name, return the typedef variable that
  // names it; otherwise, return NULL
  Variable const *getTypedefNameC() const;
  Variable *getTypedefName() { return const_cast<Variable*>(getTypedefNameC()); }
  bool hasName() const { return scopeKind==SK_CLASS || scopeKind==SK_NAMESPACE; }

  // true if this scope encloses (has as a nested scope) 's'
  bool encloses(Scope const *s) const;

  // stuff for using-directives
  void addUsingEdge(Scope *target);
  void addUsingEdgeTransitively(Env &env, Scope *target);

  // indication of scope open/close so we can maintain the
  // connection between "using" and "active using" edges
  void openedScope(Env &env);
  void closedScope();

  // dsw: needed this and this was a natural place to put it
  bool immediateGlobalScopeChild();
  bool linkerVisible();
  // This is just a unique and possibly human readable string; it is
  // used in the Oink linker imitator.
  string fullyQualifiedName();
  
  // for debugging, a quick description of this scope
  string desc() const;
};


#endif // CC_SCOPE_H
