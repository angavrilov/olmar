// cc_env.h            see license.txt for copyright and terms of use
// Env class, which is the compile-time C++ environment

#ifndef CC_ENV_H
#define CC_ENV_H

#include "cc_type.h"      // Type, AtomicType, etc. (r)
#include "strobjdict.h"   // StrObjDict
#include "strsobjdict.h"  // StrSObjDict
#include "owner.h"        // Owner
#include "exc.h"          // xBase
#include "sobjlist.h"     // SObjList
#include "objstack.h"     // ObjStack
#include "sobjstack.h"    // SObjStack
#include "cc.ast.gen.h"   // C++ ast components
#include "variable.h"     // Variable (r)

class StringTable;        // strtable.h
class CCLang;             // cc_lang.h


// an error message from the typechecker; I plan to expand
// this to contain lots of information about the error, but
// for now it's just a string like every other typechecker
// produces
class ErrorMsg {
public:
  string msg;
  SourceLocation loc;
  
  // when this is true, the error message should be considered
  // when disambiguation; when it's false, it's not a sufficiently
  // severe error to warrant discarding an ambiguous alternative;
  // for the most part, only environment lookup failures are
  // considered to disambiguate
  bool disambiguates;

public:
  ErrorMsg(char const *m, SourceLocation const &L, bool d=false)
    : msg(m), loc(L), disambiguates(d) {}
  ~ErrorMsg();
  
  string toString() const;
};



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
  CompoundType *curCompound;     // CompoundType we're building
  AccessKeyword curAccess;       // access disposition in effect
  Function *curFunction;         // Function we're analyzing
  SourceLocation curLoc;         // latest AST location marker seen

public:
  Scope(int changeCount, SourceLocation const &initLoc);
  ~Scope();
};


// the entire semantic analysis state
class Env {
private:     // data
  // stack of lexical scopes; first is innermost
  ObjList<Scope> scopes;

  // list of named scopes (i.e. namespaces)
  //StringObjDict<Scope> namespaces;    // not implemented yet

public:      // data
  // stack of error messages; the first one is the latest
  // one inserted; during disambiguation, I'll remember where
  // the top was before each alternative, so I can leave this
  // stack with only the ones from one particular interpretation
  ObjList<ErrorMsg> errors;

  // string table for making new strings
  StringTable &str;

  // language options in effect
  CCLang &lang;

public:      // function
  Env(StringTable &str, CCLang &lang);
  ~Env();

  int getChangeCount() const { return scopeC()->changeCount; }

  // scopes
  void enterScope();
  void exitScope();
  Scope *scope() { return scopes.first(); }
  Scope const *scopeC() const { return scopes.firstC(); }

  // source location tracking
  void setLoc(SourceLocation const &loc);    // sets scope()->curLoc
  SourceLocation const &loc() const;         // gets scope()->curLoc
  string locStr() const { return loc().toString(); }

  // insertion into the current scope; return false if the
  // name collides with one that is already there
  bool addVariable(Variable *v);
  bool addCompound(CompoundType *ct);
  bool addEnum(EnumType *et);

  // lookup in the environment (all scopes)
  Variable *lookupPQVariable(PQName const *name) const;
  Variable *lookupVariable(StringRef name, bool innerOnly) const;
  CompoundType *lookupPQCompound(PQName const *name) const;
  CompoundType *lookupCompound(StringRef name, bool innerOnly) const;
  EnumType *lookupPQEnum(PQName const *name) const;

  // diagnostic reports; all return ST_ERROR type
  Type const *error(char const *msg, bool disambiguates=false);
  Type const *warning(char const *msg);
  Type const *unimp(char const *msg);

};


























#if 0
// thrown by some error functions
class XError : public xBase {
public:
  XError(char const *msg) : xBase(msg) {}
  XError(XError const &obj) : xBase(obj) {}
};


// elements of the environment which are scoped
class ScopedEnv {
public:
  // variables: map name -> Type
  StringSObjDict<Variable> variables;

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
  int warnings;

  // stack of compounds being constructed
  SObjList<CompoundType> compoundStack;

  // current function
  Function *currentFunction;

  // stack of source locations considered 'current'
  SObjStack<SourceLocation /*const*/> locationStack;

public:     // data
  // true in predicate expressions
  bool inPredicate;

  // string table for making up new names
  StringTable &strTable;

  // language options
  CCLang &lang;

private:    // funcs
  void grab(Type const *t) {}
  void grabAtomic(AtomicType const *t) {}

  Env(Env&);               // not allowed

public:     // funcs
  // empty toplevel environment
  Env(StringTable &table, CCLang &lang);
  virtual/*silence gcc warning*/ ~Env();

  // scope manipulation
  void enterScope();
  void leaveScope();

  // misc
  StringRef str(char const *s) const { return strTable.add(s); }
  
  // ------------- variables -------------
  // add a new variable to the innermost scope; it is an error
  // if a variable by this name already exists; 'decl' actually
  // has the name too, but I leave the 'name' parameter since
  // conceptually we're binding the name
  void addVariable(StringRef name, Variable *decl);

  // return the associated Variable structure for a variable;
  // return NULL if no such variable; if 'innerOnly' is set, we
  // only look in the innermost scope
  Variable *getVariable(StringRef name, bool innerOnly=false);

  // ----------- typedefs -------------
  // add a new typedef; error to collide;
  // after this call, the program can use 'name' as an alias for 'type'
  void addTypedef(StringRef name, Type const *type);

  // return named type or NULL if no mapping
  Type const *getTypedef(StringRef name);

  // -------------- compounds --------------
  // add a new compound; error to collide
  CompoundType *addCompound(StringRef name, CompoundType::Keyword keyword);

  // add a new field to an existing compound; error to collide
  void addCompoundField(CompoundType *ct, Variable *decl);

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
  EnumType::Value *addEnumerator(StringRef name, EnumType *et, 
                                 int value, Variable *decl);

  // lookup; return NULL if no such variable
  EnumType::Value *getEnumerator(StringRef name);


  // ------------------ error/warning reporting -----------------
  // report an error ('str' should *not* have a newline)
  virtual Type const *err(char const *str);    // returns fixed(ST_ERROR)
  void warn(char const *str);

  // versions which explicitly specify a location
  void errLoc(SourceLocation const &loc, char const *str);
  void warnLoc(SourceLocation const &loc, char const *str);

  // report an error, and throw an exception
  void errThrow(char const *str);

  // if 'condition' is true, report error 'str' and also throw an exception
  void errIf(bool condition, char const *str);

  // # reported errors
  int getErrors() const { return errors; }


  // ------------------- translation context ----------------
  void pushStruct(CompoundType *ct)     { compoundStack.prepend(ct); }
  void popStruct()                      { compoundStack.removeAt(0); }

  void setCurrentFunction(Function *f)  { currentFunction = f; }
  Function *getCurrentFunction()        { return currentFunction; }
  Type const *getCurrentRetType();

  bool isGlobalEnv() const              { return scopes.count() <= 1; }

  void pushLocation(SourceLocation const *loc);
  void popLocation()                    { locationStack.pop(); }
  SourceLocation currentLoc() const;


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

  // make a ptr-to-'type' type; returns generic Type instead of
  // PointerType because sometimes I return fixed(ST_ERROR)
  Type const *makePtrOperType(PtrOper op, CVFlags cv, Type const *type);
  Type const *makePtrType(Type const *type)
    { return makePtrOperType(PO_POINTER, CV_NONE, type); }
  Type const *makeRefType(Type const *type)
    { return makePtrOperType(PO_REFERENCE, CV_NONE, type); }

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
  Type const *promoteTypes(BinaryOp op, Type const *t1, Type const *t2);

  
  // -------------- debugging -------------
  string toString() const;
  void selfCheck() const;
};
#endif // 0


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
