// cc_env.h
// Env class, which is the compile-time C++ environment

#ifndef __CC_ENV_H
#define __CC_ENV_H

#include "cc_type.h"      // Type, AtomicType, etc.
#include "cc_err.h"       // SemanticError
#include "strobjdict.h"   // StrObjDict
#include "strsobjdict.h"  // StrSObjDict


// set of declaration modifiers present
enum DeclFlags {
  DF_NONE        = 0x0000,
  DF_INLINE      = 0x0001,
  DF_VIRTUAL     = 0x0002,
  DF_FRIEND      = 0x0004,
  DF_MUTABLE     = 0x0008,
  DF_TYPEDEF     = 0x0010,
  DF_AUTO        = 0x0020,
  DF_REGISTER    = 0x0040,
  DF_STATIC      = 0x0080,
  DF_EXTERN      = 0x0100,
  ALL_DECLFLAGS  = 0x01FF,
};


// thing to which a name is bound and for which runtime
// storage is (usually) allocated
class Variable {
public:     // data
  DeclFlags declFlags;       // inline, etc.
  Type const *type;          // type of this variable

  // the name is stored in the dictionary mapping below

public:     // funcs
  Variable(DeclFlags d, Type const *t)
    : declFlags(d), type(t) {}
};


// C++ compile-time binding environment
class Env {
private:    // data
  // built-in types -- one global immutable array, indexed
  // by SimpleTypeId
  static SimpleType const *simpleBuiltins;
  
  // and another for wrappers around them
  static CVAtomicType const *builtins;

  // parent environment; failed lookups in this environment go
  // to the parent before failing altogether
  Env * const parent;

  // counter for synthesizing names of anonymous structures;
  // only the counter in the toplevel environment is used
  int anonCounter;

  // user-defined compounds
  StringObjDict<CompoundType> compounds;

  // user-defined enums
  StringObjDict<EnumType> enums;

  // user-defined typedefs
  StringSObjDict<Type /*const*/> typedefs;

  // variables
  StringObjDict<Variable> variables;

  // intermediate types; their presence on this list is simply
  // so they will be deallocated when the environment goes away
  ObjList<Type> intermediates;

  // list of errors found so far
  ObjList<SemanticError> errors;

private:    // funcs
  void grab(Type *t);
  string makeAnonName();
  ostream& indent(ostream &os) const;

  Env(Env&);              // not allowed

public:     // funcs
  Env();                  // empty toplevel environment
  Env(Env *parent);       // nested environment
  ~Env();

  // given an AtomicType, wrap it in a CVAtomicType
  // with no const or volatile qualifiers
  CVAtomicType *makeType(AtomicType const *atomic);

  // given a type, qualify it with 'cv'; return NULL
  // if the base type cannot be so qualified
  Type const *applyCVToType(CVFlags cv, Type const *baseType);

  // make a ptr-to-'type' type
  PointerType *makePtrOperType(PtrOper op, CVFlags cv, Type const *type);

  // make a function type; initially, its parameter list is
  // empty, but can be built up by modifying the returned object
  FunctionType *makeFunctionType(Type const *retType, CVFlags cv);

  // make an array type, either of known or unknown size
  ArrayType *makeArrayType(Type const *eltType, int size);
  ArrayType *makeArrayType(Type const *eltType);

  // lookup a compound type; if it doesn't exist, declare a new
  // incomplete type, using 'keyword'; if it does, but the keyword
  // is different from its existing declaration, return NULL
  CompoundType *lookupOrMakeCompound(char const *name, CompoundType::Keyword keyword);

  // just do lookup, and return NULL if doesn't exist or is
  // declared as something other than a compound
  CompoundType *lookupCompound(char const *name);

  // lookup an enum; return NULL if not declared, or if the
  // name is declared as something other than an enum
  EnumType *lookupEnum(char const *name);

  // create a new enum type
  EnumType *makeEnumType(char const *name);

  // map a simple type into its CVAtomicType (with no const or
  // volatile) representative
  CVAtomicType const *getSimpleType(SimpleTypeId st);

  // lookup an existing type; if it doesn't exist, return NULL
  Type const *lookupType(char const *name);

  // install a new name->type binding in the environment; return
  // false if there is already a binding for this name
  bool declareVariable(char const *name, DeclFlags flags, Type const *type);

  // return true if the named variable is declared as something
  bool isDeclaredVar(char const *name);

  // report an error
  void report(SemanticError const &err);

  // get errors accumulated, including parent environments
  int numErrors() const;
  void printErrors(ostream &os) const;
  
  // just deal with errors in this environment
  int numLocalErrors() const { return errors.count(); }
  void printLocalErrors(ostream &os) const;
  void forgetLocalErrors();

  // print local errors and then throw them away
  void flushLocalErrors(ostream &os);
};


#endif // __CC_ENV_H
