// cc_env.h
// Env class, which is the compile-time C++ environment

#ifndef CC_ENV_H
#define CC_ENV_H

#include "cc_type.h"      // Type, AtomicType, etc.
#include "cc_err.h"       // SemanticError
#include "strobjdict.h"   // StrObjDict
#include "strsobjdict.h"  // StrSObjDict
#include "arraymap.h"     // ArrayMap
#include "mlvalue.h"      // MLValue
#include "owner.h"        // Owner

// fwds to other files
class DataflowEnv;        // dataflow.h
class CCTreeNode;         // cc_tree.h
class CilExpr;            // cil.h

// fwds for file
class TypeEnv;
class VariableEnv;


// type of variable identifiers
typedef int VariableId;
enum { NULL_VARIABLEID = -1, FIRST_VARIABLEID = 0 };


// thing to which a name is bound and for which runtime
// storage is (usually) allocated
class Variable {
public:     // data
  VariableId id;             // unique name (unique among .. ?)
  string name;               // declared name
  DeclFlags declFlags;       // inline, etc.
  Type const *type;          // type of this variable
  int enumValue;             // if isEnumValue(), its numerical value
  //Owner<CilExpr> initVal;    // for global variables, the initial value
  int fieldNum;              // orders fields in structs

public:     // funcs
  Variable(char const *n, DeclFlags d, Type const *t, int fieldNum);
  ~Variable();

  // some ad-hoc thing
  string toString() const;

  // ML eval() format
  //MLValue toMLValue() const;

  // when true:
  //   - 'name' is the name of an enum constant value
  //   - 'type' points to the enum itself
  //   - 'enumValue' gives the constant value
  bool isEnumValue() const { return declFlags & DF_ENUMVAL; }

  bool isGlobal() const { return declFlags & DF_GLOBAL; }

  bool isInitialized() const { return declFlags & DF_INITIALIZED; }
  void sayItsInitialized() { declFlags = (DeclFlags)(declFlags | DF_INITIALIZED); }
};


// C++ compile-time binding environment
class Env {
private:    // data
  // parent environment; failed lookups in this environment go
  // to the parent before failing altogether
  Env * const parent;               // (serf)

  // type environment; points directly to the global
  // type env, even if this is a nested-scope Env
  TypeEnv * const typeEnv;          // (serf)

  // variable environment; variable decls are stored there
  VariableEnv * const varEnv;       // (serf)

  // counter for synthesizing names; only the counter in the toplevel
  // environment is used
  int nameCounter;

  // user-defined compounds
  StringSObjDict<CompoundType> compounds;

  // user-defined enums
  StringSObjDict<EnumType> enums;

  // user-defined typedefs
  StringSObjDict<Type /*const*/> typedefs;

  // variables
  int numVariables;      // current size of 'variables'
  StringSObjDict<Variable> variables;

  // list of errors found so far
  ObjList<SemanticError> errors;

  // true during a disambiguation trial
  bool trialBalloon;

  // pointer to current dataflow environment
  DataflowEnv *denv;                       // (serf)

  // (debugging) reference count of # of nested environments pointing at me
  int referenceCt;

private:    // funcs
  void grab(Type *t);
  void grabAtomic(AtomicType *t);
  int makeFreshInteger();
  ostream& indent(ostream &os) const;
  Variable *addVariable(char const *name, DeclFlags flags, Type const *type);

  Env(Env&);               // not allowed

public:     // funcs
  // empty toplevel environment
  Env(DataflowEnv *denv, TypeEnv *typeEnv, VariableEnv *varEnv);

  // nested environment
  Env(Env *parent, TypeEnv *typeEnv, VariableEnv *env);

  ~Env();


  // added or changed during most recent rewrite
  void err(char const *str);
  void errIf(bool condition, char const *str);
  Type const *lookupTypedef(StringRef name);
  EnumType *lookupOrMakeEnum(StringRef name);
  void pushStruct(CompoundType *ct);
  void popStruct();
  void addEnumerator(StringRef name, EnumType *et, int value);
  void addTypedef(StringRef name, Type const *type);
  Variable *declareVariable(StringRef name,
                            DeclFlags flags, Type const *type);
  Type const *getCurrentRetType();
  void checkCoercible(Type const *src, Type const *dest);
  Type const *promoteTypes(Type const *t1, Type const *t2);



  // create a new environment based on the current one, purely for
  // scoping purposes
  Env *newScope();

  // create a nested environment with a different
  // place for storing variable declarations
  Env *newVariableScope(VariableEnv *venv);

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

  // given an AtomicType, wrap it in a CVAtomicType
  // with specified const or volatile qualifiers
  CVAtomicType *makeCVType(AtomicType const *atomic, CVFlags cv);

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
  FunctionType *makeFunctionType(Type const *retType/*, CVFlags cv*/);
  
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
  CompoundType *lookupOrMakeCompound(StringRef name, CompoundType::Keyword keyword);

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
  CVAtomicType const *getSimpleType(SimpleTypeId st);

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
  StringSObjDict<Variable> &getVariables() { return variables; }
  DataflowEnv &getDenv() { return *denv; }

  // access to variables that's somewhat nicer syntactically,
  // if also somewhat less efficient
  int getNumVariables() const { return numVariables; }
  Variable *getNthVariable(int n) const;

  // support for translation
  // make up a fresh variable name and install it into
  // the environment with the given type
  Variable *newTmpVar(Type const *type);

  // true if this is the toplevel, global environment
  bool isGlobalEnv() const { return parent==NULL; }

  TypeEnv *getTypeEnv() { return typeEnv; }
  VariableEnv *getVarEnv() { return varEnv; }

  // debugging
  string toString() const;
  void selfCheck() const;
};


#if 0
// --------------------- TypeEnv ------------------
// toplevel environment that owns all the types
class TypeEnv {
private:     // data
  ArrayMap<Type> types;               // TypeId -> Type*
  ArrayMap<AtomicType> atomicTypes;   // AtomicTypeId -> AtomicType*

public:
  TypeEnv();
  ~TypeEnv();

  int numTypes() const { return types.count(); }
  TypeId grab(Type *type);
  Type *lookup(TypeId id) { return types.lookup(id); }
  Type const *lookupC(TypeId id) const { return types.lookupC(id); }

  int numAtomicTypes() const { return atomicTypes.count(); }
  AtomicTypeId grabAtomic(AtomicType *type);
  AtomicType *lookupAtomic(AtomicTypeId id) { return atomicTypes.lookup(id); }
  AtomicType const *lookupAtomicC(AtomicTypeId id) const { return atomicTypes.lookupC(id); }
  
  void empty() { types.empty(); atomicTypes.empty(); }
};


// ------------------ VariableEnv ---------------
// something to own variable decls; current plan is to have
// one for globals and then one for each function body
class VariableEnv {
private:
  ArrayMap<Variable> vars;

public:
  VariableEnv();
  ~VariableEnv();

  int numVars() const { return vars.count(); }
  VariableId grab(Variable * /*owner*/ var);
  Variable *lookup(VariableId id) { return vars.lookup(id); }
  Variable const *lookupC(VariableId id) const { return vars.lookupC(id); }
  Variable *&lookupRef(VariableId id) { return vars.lookupRef(id); }
  void empty() { vars.empty(); }
  
  // only for use by the iterator macro
  ArrayMap<Variable> const &getVars() const { return vars; }
};


#define FOREACH_VARIABLE(env, var) \
  FOREACH_ARRAYMAP(Variable, (env).getVars(), var)

#endif // 0


#endif // CC_ENV_H
