// cc_scope.h            see license.txt for copyright and terms of use
// a C++ scope, which is used by the Env parsing environment
// and also by CompoundType to store members

#ifndef CC_SCOPE_H
#define CC_SCOPE_H

#include "strsobjdict.h"  // StrSObjDict
#include "cc_flags.h"     // AccessKeyword
#include "fileloc.h"      // SourceLocation
#include "strtable.h"     // StringRef
#include "sobjlist.h"     // SObjList

class Env;                // cc_env.h
class Variable;           // variable.h
class CompoundType;       // cc_type.h
class EnumType;           // cc_type.h
class Function;           // cc.ast
class TemplateParams;     // cc_type.h
class PQName;             // cc.ast


// variable lookup sometimes has complicated exceptions or
// special cases, so I'm folding lookup options into one value
enum LookupFlags {
  LF_NONE            = 0,
  LF_INNER_ONLY      = 0x01,    // only look in the innermost scope
  LF_ONLY_TYPES      = 0x02,    // ignore (skip over) non-type names

  LF_ALL_FLAGS       = 0x03,    // bitwise OR of all flags
};

// experiment: will this work?
inline LookupFlags operator| (LookupFlags f1, LookupFlags f2)
  { return (LookupFlags)((int)f1 | (int)f2); }
inline LookupFlags operator& (LookupFlags f1, LookupFlags f2)
  { return (LookupFlags)((int)f1 & (int)f2); }
inline LookupFlags operator~ (LookupFlags f)
  { return (LookupFlags)(~(int)f); }


// information about a single scope: the names defined in it,
// any "current" things being built (class, function, etc.)
class Scope {
private:     // data
  // ----------------- name spaces --------------------
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
  // the scope stack to insert the name (used for environments of
  // template parameters)
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

  // true for function parameter list scopes; the C++ standard
  // has places where they are treated differently, e.g. 3.3.1 para 5
  bool isParameterListScope;

  // ------------- "current" entities -------------------
  // these are set to allow the typechecking code to know about
  // the context we're in
  CompoundType *curCompound;          // (serf) CompoundType we're building
  AccessKeyword curAccess;            // access disposition in effect
  Function *curFunction;              // (serf) Function we're analyzing
  TemplateParams *curTemplateParams;  // (owner) params to attach to next function or class
  SourceLocation curLoc;              // latest AST location marker seen
                                    
private:     // funcs
  Variable const *lookupPQVariableC(PQName const *name, bool &crossVirtual,
                                    Env &env, LookupFlags f) const;

public:      // funcs
  Scope(int changeCount, SourceLocation const &initLoc);
  ~Scope();

  int getChangeCount() const { return changeCount; }

  // insertion; these return false if the corresponding map already
  // has a binding (unless 'forceReplace' is true)
  bool addVariable(Variable *v, bool forceReplace=false);
  bool addCompound(CompoundType *ct);
  bool addEnum(EnumType *et);

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
};


#endif // CC_SCOPE_H
