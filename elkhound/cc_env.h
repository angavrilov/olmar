// cc_env.h
// Env class, which is the compile-time C++ environment

#ifndef __CC_ENV_H
#define __CC_ENV_H

#include "cc_type.h"      // Type, AtomicType, etc.
#include "cc_err.h"       // SemanticError
#include "strobjdict.h"   // StrObjDict
#include "strsobjdict.h"  // StrSObjDict

class DataflowEnv;        // dataflow.h
class CCTreeNode;         // cc_tree.h


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
  DF_ENUMVAL     = 0x0200,    // not really a decl flag, but a Variable flag..
  ALL_DECLFLAGS  = 0x03FF,
};


// thing to which a name is bound and for which runtime
// storage is (usually) allocated
class Variable {
public:     // data
  string name;               // declared name
  DeclFlags declFlags;       // inline, etc.
  Type const *type;          // type of this variable
  int enumValue;             // if isEnumValue(), its numerical value
  bool initialized;          // true if has been declared with an initializer (or, for functions, with code)

public:     // funcs
  Variable(char const *n, DeclFlags d, Type const *t)
    : name(n), declFlags(d), type(t), enumValue(0), initialized(false) {}

  string toString() const;

  // when true:
  //   - 'name' is the name of an enum constant value
  //   - 'type' points to the enum itself
  //   - 'enumValue' gives the constant value
  bool isEnumValue() const { return declFlags & DF_ENUMVAL; }
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
  Env * const parent;               // (serf)

  // counter for synthesizing names; only the counter in the toplevel
  // environment is used
  int nameCounter;

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

  // true during a disambiguation trial
  bool trialBalloon;

  // pointer to current dataflow environment
  DataflowEnv *denv;                       // (serf)

  // (debugging) reference count of # of nested environments pointing at me
  int referenceCt;

private:    // funcs
  Type *grab(Type *t);
  int makeFreshInteger();
  ostream& indent(ostream &os) const;
  Variable *addVariable(char const *name, DeclFlags flags, Type const *type);

  Env(Env&);               // not allowed

public:     // funcs
  Env(DataflowEnv *denv);  // empty toplevel environment
  Env(Env *parent);        // nested environment
  ~Env();

  // close this environment's link with its parent; this must
  // intended to be done before either is deallocated (it is
  // automatically done in ~Env, but sometimes we need to
  // deallocate the parent before the child)
  void killParentLink();
  
  // naming helpers
  string makeAnonName();
  string makeFreshName(char const *prefix);

  // there doesn't seem to be much rhyme or reason to why some return
  // const and others don't .. but some definitely need to *not*
  // return const (like makeFunctionType), so whatever

  // given an AtomicType, wrap it in a CVAtomicType
  // with no const or volatile qualifiers
  CVAtomicType *makeType(AtomicType const *atomic);

  // given a type, qualify it with 'cv'; return NULL
  // if the base type cannot be so qualified
  Type const *applyCVToType(CVFlags cv, Type const *baseType);

  // given an array type with no size, return one that is
  // the same except its size is as specified
  ArrayType const *setArraySize(ArrayType const *type, int size);

  // make a ptr-to-'type' type
  PointerType *makePtrOperType(PtrOper op, CVFlags cv, Type const *type);

  // make a function type; initially, its parameter list is
  // empty, but can be built up by modifying the returned object
  FunctionType *makeFunctionType(Type const *retType, CVFlags cv);
  
  // sometimes it's handy to specify all args at once
  FunctionType *makeFunctionType_1arg(
    Type const *retType, CVFlags cv,
    Type const *arg1Type, char const *arg1name);

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

  // add an enum value
  void addEnumValue(CCTreeNode const *node, char const *name, 
                    EnumType const *type, int value);

  // map a simple type into its CVAtomicType (with no const or
  // volatile) representative
  static CVAtomicType const *getSimpleType(SimpleTypeId st);

  // lookup an existing type; if it doesn't exist, return NULL
  Type const *lookupLocalType(char const *name);
  Type const *lookupType(char const *name);

  // install a new name->type binding in the environment;
  // throw an XSemanticError exception if there is a problem;
  // if ok, return the Variable created (or already existant,
  // if it's allowed); returns NULL for typedefs
  Variable *declareVariable(CCTreeNode const *node, char const *name,
                            DeclFlags flags, Type const *type,
                            bool initialized);

  // return true if the named variable is declared as something
  bool isLocalDeclaredVar(char const *name);
  bool isDeclaredVar(char const *name);

  // some enum junk
  bool isEnumValue(char const *name);

  // return the associated Variable structure for a variable;
  // throws an exception if it doesn't exist
  Variable *getVariable(char const *name);
                                            
  // same, but return NULL on failure instead
  Variable *getVariableIf(char const *name);

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
  
  // trial balloon flag
  void setTrialBalloon(bool val) { trialBalloon = val; }
  bool isTrialBalloon() const;
  
  // support for analysis routines
  StringObjDict<Variable> &getVariables() { return variables; }
  DataflowEnv &getDenv() { return *denv; }

  // support for translation
  // make up a fresh variable name and install it into
  // the environment with the given type
  Variable *newTmpVar(Type const *type);

  // debugging
  string toString() const;
};


#endif // __CC_ENV_H
