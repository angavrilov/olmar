// cc_scope.h            see license.txt for copyright and terms of use
// a C++ scope, which is used by the Env parsing environment
// and also by CompoundType to store members

#ifndef CC_SCOPE_H
#define CC_SCOPE_H

#include "cc_flags.h"     // AccessKeyword
#include "srcloc.h"       // SourceLoc
#include "strtable.h"     // StringRef
#include "sobjlist.h"     // SObjList
#include "array.h"        // ArrayStack
#include "serialno.h"     // INHERIT_SERIAL_BASE
#include "strmap.h"       // StringRefMap

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
  LF_INNER_ONLY      = 0x001,   // only look in the innermost scope
  LF_ONLY_TYPES      = 0x002,   // ignore (skip over) non-type names
  LF_TYPENAME        = 0x004,   // user used 'typename' keyword
  LF_SKIP_CLASSES    = 0x008,   // skip class scopes
  LF_ONLY_NAMESPACES = 0x010,   // ignore non-namespace names
  LF_TYPES_NAMESPACES= 0x020,   // ignore non-type, non-namespace names
  LF_QUALIFIED       = 0x040,   // context is a qualified lookup
  LF_TEMPL_PRIMARY   = 0x080,   // return template primary rather than instantiating it
  LF_IMPL_DECL_FUNC  = 0x100,   // K&R implicitly declare functions that are not found
  LF_DECLARATOR      = 0x200,   // context is a declarator name lookup; for templates, this means to pick the primary or a specialization, but don't instantiate
  LF_SELFNAME        = 0x400,   // the DF_SELFNAME is visible
  LF_DEPENDENT       = 0x800,   // the lookup is in a dependent context

  LF_ALL_FLAGS       = 0xFFF,   // bitwise OR of all flags
};

ENUM_BITWISE_OPS(LookupFlags, LF_ALL_FLAGS)     // smbase/macros.h


// information about a single scope: the names defined in it,
// any "current" things being built (class, function, etc.)
class Scope INHERIT_SERIAL_BASE {
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
  StringRefMap<Variable> variables;

  // compounds: map name -> CompoundType
  StringRefMap<CompoundType> compounds;

  // enums: map name -> EnumType
  StringRefMap<EnumType> enums;

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
                                      
  // If this is a template (SK_TEMPLATE_PARAMS) scope, these are the
  // template parameters.  We will attach them to functions and
  // classes contained in this scope, as those functions and classes
  // are parameterized by these variables.
  SObjList<Variable> templateParams;

  // If this is SK_TEMPLATE_PARAMS, then the parameters correspond to
  // some specific template primary.  This pointer names that primary.
  // It is initially NULL, as we don't immediately know which is being
  // parameterized, but is set to non-NULL as soon as we know.
  Variable *parameterizedPrimary;          // (nullable serf)

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
  SourceLoc curLoc;                   // latest AST location marker seen

private:     // funcs
  void lookupPQVariableC_considerBase
    (PQName const *name, Env &env, LookupFlags flags,
     Variable const *&v1, CompoundType const *&v1Base,
     BaseClassSubobj const *v2Subobj) const;
  void lookupPQEnumC_considerBase
    (PQName const *name, Env &env, LookupFlags flags,
     EnumType const *&v1,
     CompoundType const *&v1Base,
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

  // this is actually for debugging only ....
  int getNumVariables() const       { return variables.getNumEntries(); }

  // some syntactic sugar on the scope kind
  bool isGlobalScope() const        { return scopeKind == SK_GLOBAL; }
  bool isParameterScope() const     { return scopeKind == SK_PARAMETER; }
  bool isFunctionScope() const      { return scopeKind == SK_FUNCTION; }
  bool isClassScope() const         { return scopeKind == SK_CLASS; }
  bool isTemplateParamScope() const { return scopeKind == SK_TEMPLATE_PARAMS; }
  bool isTemplateArgScope() const   { return scopeKind == SK_TEMPLATE_ARGS; }
  bool isNamespace() const          { return scopeKind == SK_NAMESPACE; }

  // template params or args ...
  bool isTemplateScope() const      { return isTemplateParamScope() || isTemplateArgScope(); }

  // true if this scope is guaranteed to stick around until the end of
  // the translation unit, and hence child scopes' 'parentScope' field
  // and Variable::scope should be set to point at it
  bool isPermanentScope() const;

  // are we in a template scope that is in a global scope?
  bool isGlobalTemplateScope() const;
  // are we in a scope that at some point above is an uninstantiated
  // templatized scope?
  bool isWithinUninstTemplate() const;

  // True if this scope has extant template parameters.
  //
  // Actually, I think the places that call this are really just
  // asking whether this is an SK_TEMPLATE_PARAMS, because an empty
  // template parameter list is treated uniformly.
  bool hasTemplateParams() const { return isTemplateParamScope(); }

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
  EnumType const *lookupEnumC(StringRef name, Env &env, LookupFlags f=LF_NONE) const;

  // lookup of a possibly-qualified name; used for member access
  // like "a.B::f()"
  Variable const *lookupPQVariableC(PQName const *name, Env &env, LookupFlags f=LF_NONE) const;
  EnumType const *lookupPQEnumC(PQName const *name, Env &env, LookupFlags flags) const;

  // non-const versions..
  Variable *lookupVariable(StringRef name, Env &env, LookupFlags f=LF_NONE)
    { return const_cast<Variable*>(lookupVariableC(name, env, f)); }
  CompoundType *lookupCompound(StringRef name, LookupFlags f=LF_NONE)
    { return const_cast<CompoundType*>(lookupCompoundC(name, f)); }
  EnumType *lookupEnum(StringRef name, Env &env, LookupFlags f=LF_NONE)
    { return const_cast<EnumType*>(lookupEnumC(name, env, f)); }
  Variable *lookupPQVariable(PQName const *name, Env &env, LookupFlags f=LF_NONE)
    { return const_cast<Variable*>(lookupPQVariableC(name, env, f)); }

  // for iterating over the variables
  StringRefMap<Variable>::Iter getVariableIter() const
    { return StringRefMap<Variable>::Iter(variables); }

  // and the inner classes
  StringRefMap<CompoundType>::Iter getCompoundIter() const
    { return StringRefMap<CompoundType>::Iter(compounds); }

  // lookup within the 'variables' map, without consulting base
  // classes, etc.; returns NULL if not found
  Variable *rawLookupVariable(char const *name)
    { return variables.get(name); }

  // if this scope has a name, return the typedef variable that
  // names it; otherwise, return NULL
  Variable const *getTypedefNameC() const;
  Variable *getTypedefName() { return const_cast<Variable*>(getTypedefNameC()); }
  bool hasName() const { return scopeKind==SK_CLASS || scopeKind==SK_NAMESPACE; }

  // true if this scope encloses (has as a nested scope) 's'; this
  // is proper enclosure: it is not the case that s->encloses(s)
  bool encloses(Scope const *s) const;
  
  // non-proper enclosure
  bool enclosesOrEq(Scope const *s) const;

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
  //
  // sm: TODO: Change the name so it reflects the mangling activity;
  // I want "fullyQualifiedName" to do what "fullyQualifiedCName"
  // does now.
  string fullyQualifiedName(bool mangle = true);
  
  // more C-like notation for a fully qualified name
  string fullyQualifiedCName()
    { return fullyQualifiedName(false /*mangle*/); }

  // set 'parameterizedPrimary', checking a few things in the process
  void setParameterizedPrimary(Variable *primary);
       
  // for debugging, a quick description of this scope
  string desc() const;
  void gdb() const;
};


#endif // CC_SCOPE_H
