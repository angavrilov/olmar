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

#include "fileloc.h"           // SourceLocation
#include "strtable.h"          // StringRef
#include "cc_flags.h"          // DeclFlags
#include "sobjlist.h"          // SObjList
#include "variable_annot.h"    // VariableAnnot

class Type;                    // cc_type.h
class OverloadSet;             // below
class Scope;                   // scope
class Expression;              // cc.ast
class Function;                // cc.ast
class BasicTypeFactory;        // cc_type.h

class Variable : public VariableAnnot {
#if CC_QUAL
public:
  static SObjList<Variable> instances;
#endif

public:    // data
  // for now, there's only one location, and it's the definition
  // location if that exists, else the declaration location; there
  // are significant advantages to storing *two* locations (first
  // declaration, and definition), but I haven't done that yet
  SourceLocation loc;     // location of the name in the source text

  StringRef name;         // name introduced (possibly NULL for abstract declarators)
  Type *type;             // type of the variable
  DeclFlags flags;        // various flags

  // associated value for constant literals, e.g. "const int five = 5;"
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

protected:    // funcs
  friend class BasicTypeFactory;

  public:                       // dsw: Will fix this later
  Variable(SourceLocation const &L, StringRef n,
           Type *t, DeclFlags f,
           // Scott, I'll take this out but I need it for now.
           bool put_into_instances_list=true
           );

public:
  ~Variable();

  //Variable *deepClone() const;

  bool hasFlag(DeclFlags f) const { return (flags & f) != 0; }
  void setFlag(DeclFlags f) { flags = (DeclFlags)(flags | f); }
  void addFlags(DeclFlags f) { setFlag(f); }
  void clearFlag(DeclFlags f) { flags = (DeclFlags)(flags & ~f); }

  // some convenient interpretations of 'flags'
  bool hasAddrTaken() const { return flags & DF_ADDRTAKEN; }
  bool isGlobal() const { return flags & DF_GLOBAL; }

  // create an overload set if it doesn't exist, and return it
  OverloadSet *getOverloadSet();

  // some ad-hoc thing
  string toString() const;
  
  // syntax when used in a parameter list
  string toStringAsParameter() const;
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
};


// This function renders an Expression as a string, if it knows how
// to.  This function is here to cut the dependency between Types and
// the AST.  If the AST-aware modules are compiled into this program,
// then this function just calls into them, prepending the prefix; but
// if not, then this always returns "".
string renderExpressionAsString(char const *prefix, Expression const *e);


#endif // VARIABLE_H
