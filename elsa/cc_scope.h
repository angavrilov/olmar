// cc_scope.h            see license.txt for copyright and terms of use
// a C++ scope, which is used by the Env parsing environment
// and also by CompoundType to store members

#ifndef CC_SCOPE_H
#define CC_SCOPE_H

#include "strsobjdict.h"  // StrSObjDict
#include "cc_flags.h"     // AccessKeyword
#include "fileloc.h"      // SourceLocation
#include "strtable.h"     // StringRef

class Env;                // cc_env.h
class Variable;           // variable.h
class CompoundType;       // cc_type.h
class EnumType;           // cc_type.h
class Function;           // cc.ast
class TemplateParams;     // cc_type.h


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

  // ------------- "current" entities -------------------
  // these are set to allow the typechecking code to know about
  // the context we're in
  CompoundType *curCompound;       // (serf) CompoundType we're building
  AccessKeyword curAccess;         // access disposition in effect
  Function *curFunction;           // (serf) Function we're analyzing
  TemplateParams *templateParams;  // (owner) params to attach to next function or class
  SourceLocation curLoc;           // latest AST location marker seen
  
private:     // funcs
  Variable const *lookupVariableC(StringRef name, bool &crossVirtual, 
                                  bool innerOnly, Env &env) const;

public:      // funcs
  Scope(int changeCount, SourceLocation const &initLoc);
  ~Scope();

  int getChangeCount() const { return changeCount; }

  // insertion; these return false if the corresponding map already
  // has a binding (unless 'forceReplace' is true)
  bool addVariable(Variable *v, bool forceReplace=false);
  bool addCompound(CompoundType *ct);
  bool addEnum(EnumType *et);

  // lookup; these return NULL if the name isn't found
  Variable const *lookupVariableC(StringRef name, bool innerOnly, Env &env) const;
  CompoundType const *lookupCompoundC(StringRef name, bool innerOnly) const;
  EnumType const *lookupEnumC(StringRef name, bool innerOnly) const;

  // non-const versions..
  Variable *lookupVariable(StringRef name, bool innerOnly, Env &env)
    { return const_cast<Variable*>(lookupVariableC(name, innerOnly, env)); }
  CompoundType *lookupCompound(StringRef name, bool innerOnly)
    { return const_cast<CompoundType*>(lookupCompoundC(name, innerOnly)); }
  EnumType *lookupEnum(StringRef name, bool innerOnly)
    { return const_cast<EnumType*>(lookupEnumC(name, innerOnly)); }
    
  // for iterating over the variables
  StringSObjDict<Variable> getVariableIter() const
    { return StringSObjDict<Variable>(variables); }
};


#endif // CC_SCOPE_H
