// cc_env.h
// Env class, which is the compile-time C++ environment

#ifndef CC_ENV_H
#define CC_ENV_H

#include "cc_type.h"      // Type, AtomicType, etc.
#include "strobjdict.h"   // StrObjDict
#include "strsobjdict.h"  // StrSObjDict
#include "owner.h"        // Owner
#include "exc.h"          // xBase
#include "sobjlist.h"     // SObjList


// thrown by some error functions
class XError : public xBase {
public:
  XError(char const *msg) : xBase(msg) {}
  XError(XError const &obj) : xBase(obj) {}
};


// mapping from name to type, for purpose of storage instantiation
class Variable {
public:     // data
  StringRef name;            // declared name
  DeclFlags declFlags;       // inline, etc.
  Type const *type;          // type of this variable

public:     // funcs
  Variable(StringRef n, DeclFlags d, Type const *t);
  ~Variable();

  // some ad-hoc thing
  string toString() const;

  // ML eval() format
  //MLValue toMLValue() const;

  bool isGlobal() const { return declFlags & DF_GLOBAL; }
  //bool isInitialized() const { return declFlags & DF_INITIALIZED; }
  //void sayItsInitialized() { declFlags = (DeclFlags)(declFlags | DF_INITIALIZED); }
};


// elements of the environment which are scoped
class ScopedEnv {
public:
  // variables: map name -> Type
  StringObjDict<Variable> variables;
  
public:
  ScopedEnv();
  ~ScopedEnv();
};


// C++ compile-time binding environment
class Env {
private:    // data
  // ----------- fundamental maps ---------------
  // list of active scopes; element 0 is the innermost scope
  ObjList<ScopedEnv> scopes;

  // typedefs: map name -> Type
  StringSObjDict<Type /*const*/> typedefs;

  // compounds: map name -> CompoundType
  StringSObjDict<CompoundType> compounds;

  // enums: map name -> EnumType
  StringSObjDict<EnumType> enums;

  // enumerators: map name -> EnumType::Value
  StringSObjDict<EnumType::Value> enumerators;

  // -------------- miscellaneous ---------------
  // count of reported errors
  int errors;

  // stack of compounds being constructed
  SObjList<CompoundType> compoundStack;

  // current function's return type
  Type const *currentRetType;

private:    // funcs
  void grab(Type const *t) {}
  void grabAtomic(AtomicType const *t) {}

  Env(Env&);               // not allowed

public:     // funcs
  // empty toplevel environment
  Env();
  ~Env();

  // scope manipulation
  void enterScope();
  void leaveScope();

  // ------------- variables -------------
  // add a new variable to the innermost scope; it is an error
  // if a variable by this name already exists
  Variable *addVariable(StringRef name, DeclFlags flags, Type const *type);

  // return the associated Variable structure for a variable;
  // return NULL if no such variable; if 'innerOnly' is set, we
  // only look in the innermost scope
  Variable *getVariable(StringRef name, bool innerOnly=false);

  // ----------- typedefs -------------
  // add a new typedef; error to collide
  void addTypedef(StringRef name, Type const *type);
  
  // return named type or NULL if no mapping
  Type const *getTypedef(StringRef name);

  // -------------- compounds --------------
  // add a new compound; error to collide
  CompoundType *addCompound(StringRef name, CompoundType::Keyword keyword);

  // add a new field to an existing compound; error to collide
  void addCompoundField(CompoundType *ct, StringRef name, Type const *type);

  // lookup, and return NULL if doesn't exist
  CompoundType *getCompound(StringRef name);

  // lookup a compound type; if it doesn't exist, declare a new
  // incomplete type, using 'keyword'; if it does, but the keyword
  // is different from its existing declaration, return NULL
  CompoundType *getOrAddCompound(StringRef name, CompoundType::Keyword keyword);

  // ----------------- enums -----------------------
  // create a new enum type; error to collide
  EnumType *addEnum(StringRef name);

  // lookup an enum; return NULL if not declared
  EnumType *getEnum(StringRef name);

  EnumType *getOrAddEnum(StringRef name);

  // ------------------ enumerators -------------------
  // add an enum value
  EnumType::Value *addEnumerator(StringRef name, EnumType *et, int value);

  // lookup; return NULL if no such variable
  EnumType::Value *getEnumerator(StringRef name);


  // ------------------ error/warning reporting -----------------
  // report an error
  void err(char const *str);

  // report an error, and throw an exception
  void errThrow(char const *str);

  // if 'condition' is true, report error 'str' and also throw an exception
  void errIf(bool condition, char const *str);

  // # reported errors
  int getErrors() const { return errors; }


  // ------------------- translation context ----------------
  void pushStruct(CompoundType *ct)     { compoundStack.prepend(ct); }
  void popStruct()                      { compoundStack.removeAt(0); }

  void setCurrentRetType(Type const *t) { currentRetType = t; }
  Type const *getCurrentRetType()       { return currentRetType; }

  bool isGlobalEnv() const              { return scopes.count() <= 1; }


  // --------------- type construction -----------------
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

  #if 0
  // sometimes it's handy to specify all args at once
  FunctionType *makeFunctionType_1arg(
    Type const *retType, CVFlags cv,
    Type const *arg1Type, char const *arg1name);
  #endif // 0

  // make an array type, either of known or unknown size
  ArrayType *makeArrayType(Type const *eltType, int size);
  ArrayType *makeArrayType(Type const *eltType);

  // map a simple type into its CVAtomicType (with no const or
  // volatile) representative
  CVAtomicType const *getSimpleType(SimpleTypeId st);


  // --------------- type checking ----------------
  // type manipulation arising from expression semantics
  void checkCoercible(Type const *src, Type const *dest);
  Type const *promoteTypes(Type const *t1, Type const *t2);

  
  // -------------- debugging -------------
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
