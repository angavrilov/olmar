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

  LF_ALL_FLAGS       = 0x07,    // bitwise OR of all flags
};

// experiment: will this work?
inline LookupFlags operator| (LookupFlags f1, LookupFlags f2)
  { return (LookupFlags)((int)f1 | (int)f2); }
inline LookupFlags operator& (LookupFlags f1, LookupFlags f2)
  { return (LookupFlags)((int)f1 & (int)f2); }
inline LookupFlags operator~ (LookupFlags f)
  { return (LookupFlags)((~(int)f) & LF_ALL_FLAGS); }


// information about a single scope: the names defined in it,
// any "current" things being built (class, function, etc.)
class Scope {
private:     // data
  // ----------------- name spaces --------------------
  // variables: name -> Variable
  // note: this includes typedefs (DF_TYPEDEF is set), and it also
  // includes enumerators (DF_ENUMERATOR is set)
  StringSObjDict<Variable> variables;
  // dsw: variables in the order they were added; I need this for
  // struct initializers
  SObjList<Variable> data_variables_in_order;
  // dsw: map from names to indicies into data_variables_in_order;
  // this is needed to support compound initializeres with mixed
  // descriptor-ed and non-descriptor-ed initializers
  StringSObjDict<int> name_pos;

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

  // doh.. I need a list of compounds so I can check the inner
  // classes, and I can't seem to iterate over my StringSObjDict
  // without triggering a compiler codegen bug..
  SObjList<CompoundType> innerClasses;

  // (serf) parent named scope; presently, this is only used so that
  // inner classes can refer to their containing classes (eventually
  // nested namespaces will be supported this way too); this field is
  // only set to non-NULL after the inner class has been fully
  // constructed, since we can rely on the Environment's scope stack
  // to look up things in containing classes while building the inner
  // class for the first time
  Scope *parentScope;

  // what kind of scope is this?
  ScopeKind scopeKind;

  // ------------- "current" entities -------------------
  // these are set to allow the typechecking code to know about
  // the context we're in
  CompoundType *curCompound;          // (serf) CompoundType we're building
  AccessKeyword curAccess;            // access disposition in effect
  Function *curFunction;              // (serf) Function we're analyzing
  TemplateParams *curTemplateParams;  // (owner) params to attach to next function or class
  SourceLoc curLoc;                   // latest AST location marker seen
  TranslationUnit *tunit;             // the translation unit we are in
                                    
private:     // funcs
  void Scope::lookupPQVariableC_considerBase
    (PQName const *name, Env &env, LookupFlags flags,
     Variable const *&v1, CompoundType const *&v1Base,
     BaseClassSubobj const *v2Subobj) const;

public:      // funcs
  Scope(ScopeKind sk, int changeCount, SourceLoc initLoc, TranslationUnit *tunit0);
  ~Scope();

  int getChangeCount() const { return changeCount; }

  // some syntactic sugar on the scope kind
  bool isGlobalScope() const        { return scopeKind == SK_GLOBAL; }
  bool isParameterScope() const     { return scopeKind == SK_PARAMETER; }
  bool isFunctionScope() const      { return scopeKind == SK_TEMPLATE; }
  bool isClassScope() const         { return scopeKind == SK_CLASS; }
  bool isTemplateScope() const      { return scopeKind == SK_TEMPLATE; }

  // dsw: I need to lookup variables and I don't have an Env to pass
  // in; additionally, I'm not so sure that lookupVariable() really
  // does what I want
  StringSObjDict<Variable> &get_variables() {return variables;}
  SObjList<Variable> &get_data_variables_in_order() {return data_variables_in_order;}
  StringSObjDict<int> &get_name_pos() {return name_pos;}
  StringSObjDict<Variable> const &get_variablesC() const {return variables;}
  SObjList<Variable> const &get_data_variables_in_orderC() const {return data_variables_in_order;}

  int *get_position_of_name(StringRef id) {return get_name_pos().queryif(id);}

  // insertion; these return false if the corresponding map already
  // has a binding (unless 'forceReplace' is true)
  bool addVariable(Variable *v, bool forceReplace=false);
  bool addCompound(CompoundType *ct);
  bool addEnum(EnumType *et);

  // mark 'v' as being a member of this scope, by setting its 'scope'
  // and 'scopeKind' members (this is not done by 'addVariable')
  void registerVariable(Variable *v);

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

  // for iterating over the variables
  StringSObjDict<Variable>::IterC getVariableIter() const
    { return StringSObjDict<Variable>::IterC(variables); }

  // and the inner classes
  StringSObjDict<CompoundType>::IterC getCompoundIter() const
    { return StringSObjDict<CompoundType>::IterC(compounds); }
    
  int private_compoundTop() const
    { return compounds.private_getTopAddr(); }

  public:                       // dsw: needed this and this was a natual place to put it
  bool immediateGlobalScopeChild();
  bool linkerVisible();
  stringBuilder fullyQualifiedName();
};


#endif // CC_SCOPE_H
