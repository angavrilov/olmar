// cc_scope.h            see license.txt for copyright and terms of use
// a C++ scope, which is used by the Env parsing environment
// and also by CompoundType to store members

#ifndef CC_SCOPE_H
#define CC_SCOPE_H

#include "strsobjdict.h"  // StrSObjDict
#include "cc_flags.h"     // AccessKeyword
#include "fileloc.h"      // SourceLocation

class Env;                // cc_env.h
class Variable;           // variable.h
class CompoundType;       // cc_type.h
class EnumType;           // cc_type.h
class Function;           // cc.ast


// information about a single scope: the names defined in it,
// any "current" things being built (class, function, etc.)
class Scope {
  friend class Env;

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
  // ------------- "current" entities -------------------
  // these are set to allow the typechecking code to know about
  // the context we're in
  CompoundType *curCompound;     // CompoundType we're building
  AccessKeyword curAccess;       // access disposition in effect
  Function *curFunction;         // Function we're analyzing
  SourceLocation curLoc;         // latest AST location marker seen

public:
  Scope(int changeCount, SourceLocation const &initLoc);
  ~Scope();
};


#endif // CC_SCOPE_H
