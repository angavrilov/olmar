// variable.h                       see license.txt for copyright and terms of use
// information about a name
//
// Every binding introduction (e.g. declaration) of a name will own
// one of these to describe the introduced name; every reference to
// that name will be annotated with a pointer to the Variable hanging
// off the introduction.   
//
// The name 'variable' is a slight misnomer; it's used for naming:
//   - local and global variables
//   - logic variables (in the thmprv context)
//   - function names
//   - function parameters
//   - structure fields
//   - enumeration values
//   - typedef'd names (though these are translated-away early on)
//
// I've decided that, rather than AST nodes trying to own Variables,
// Variables will live in a separate pool (like types) so the AST
// nodes can properly share them at will.

#ifndef VARIABLE_H
#define VARIABLE_H

#include "srcloc.h"            // SourceLoc
#include "strtable.h"          // StringRef
#include "cc_flags.h"          // DeclFlags, ScopeKind
#include "sobjlist.h"          // SObjList

class Type;                    // cc_type.h
class FunctionType;            // cc_type.h
class OverloadSet;             // below
class Scope;                   // scope
class Expression;              // cc.ast
class Function;                // cc.ast
class BasicTypeFactory;        // cc_type.h

class Variable {
public:    // data
  // for now, there's only one location, and it's the definition
  // location if that exists, else the declaration location; there
  // are significant advantages to storing *two* locations (first
  // declaration, and definition), but I haven't done that yet
  SourceLoc loc;          // location of the name in the source text
                          
  // name introduced (possibly NULL for abstract declarators)
  StringRef name;        

  // type of the variable
  Type *type;             
  
  // various flags; 'const' to force modifications to go through
  // the 'setFlagsTo' method
  const DeclFlags flags;

  // associated value for constant literals, e.g. "const int five = 5;",
  // or default value for function parameters
  Expression *value;      // (nullable serf)

  // associated function definition; if NULL, either this thing isn't
  // a function or we never saw a definition
  Function *funcDefn;     // (nullable serf)

  // if this name has been overloaded, then this will be a pointer
  // to the set of overloaded names; otherwise it's NULL
  OverloadSet *overload;  // (nullable serf)

  // access control applied to this variable in the context
  // in which it appears (defaults to AK_PUBLIC)
  AccessKeyword access;

  // named scope in which the variable appears; this is only non-NULL
  // if the scope has a name, i.e. it continues to be available for
  // use even after it's lexically closed
  Scope *scope;           // (nullable serf)

  // kind of scope in which the name is declared; initially this
  // is SK_UNKNOWN
  ScopeKind scopeKind;

protected:    // funcs
  friend class BasicTypeFactory;
  Variable(SourceLoc L, StringRef n, Type *t, DeclFlags f);

public:
  virtual ~Variable();

  bool hasFlag(DeclFlags f) const { return (flags & f) != 0; }
  void setFlag(DeclFlags f) { setFlagsTo(flags | f); }
  void addFlags(DeclFlags f) { setFlag(f); }
  void clearFlag(DeclFlags f) { setFlagsTo(flags & ~f); }

  // change the value of 'flags'; this is virtual so that annotation
  // systems can monitor flag modifications
  virtual void setFlagsTo(DeclFlags f);

  // some convenient interpretations of 'flags'
  bool hasAddrTaken() const { return flags & DF_ADDRTAKEN; }
  bool isGlobal() const { return flags & DF_GLOBAL; }
  bool isStatic() const { return flags & DF_STATIC; }

  // true if this name refers to a template function, or is
  // the typedef-name of a template class
  bool isTemplate() const { return isTemplateFunction() || isTemplateClass(); }
  bool isTemplateFunction() const;
  bool isTemplateClass() const;

  // variable's type.. same as the public 'type' field..
  Type *getType() { return type; }
  Type const *getTypeC() const { return type; }

  // create an overload set if it doesn't exist, and return it
  OverloadSet *getOverloadSet();

  // number of elements in the overload set, or 1 if there is no
  // overload set
  int overloadSetSize() const;

  // some ad-hoc thing
  string toString() const;

  // syntax when used in a parameter list
  string toStringAsParameter() const;

  // fully qualified but not mangled name
  string fullyQualifiedName() const;
};

inline string toString(Variable const *v) { return v->toString(); }


class OverloadSet {
public:
  // list-as-set
  SObjList<Variable> set;
  
public:
  OverloadSet();
  ~OverloadSet();
  
  void addMember(Variable *v);
  int count() const { return set.count(); }

  Variable *findByType(FunctionType const *ft, CVFlags thisCV);
};


// This function renders an Expression as a string, if it knows how
// to.  This function is here to cut the dependency between Types and
// the AST.  If the AST-aware modules are compiled into this program,
// then this function just calls into them, prepending the prefix; but
// if not, then this always returns "".
string renderExpressionAsString(char const *prefix, Expression const *e);


#endif // VARIABLE_H
