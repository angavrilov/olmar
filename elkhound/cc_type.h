// cc_type.h
// compile-type representation of C++ types
// see types.txt

#ifndef __CC_TYPE_H
#define __CC_TYPE_H

#include "str.h"        // string
#include "objlist.h"    // ObjList
#include "mlvalue.h"    // MLValue

// other files
class Env;              // cc_env.h
class TypeEnv;          // cc_env.h
class VariableEnv;      // cc_env.h
class Variable;         // cc_env.h

// fwd in this file
class SimpleType;
class CompoundType;
class EnumType;
class CVAtomicType;
class PointerType;
class FunctionType;
class ArrayType;


// set: which of "const" and/or "volatile" is specified
// I leave the lower 8 bits to represent SimpleTypeId, so I can
// freely OR them together during parsing
enum CVFlags {
  CV_NONE     = 0x000,
  CV_CONST    = 0x100,
  CV_VOLATILE = 0x200,
  CV_OWNER    = 0x400,     // experimental extension
  CV_ALL      = 0x700,
  NUM_CVFLAGS = 3
};

extern char const * const cvFlagNames[NUM_CVFLAGS];      // 0="const", 1="volatile", 2="owner"
string toString(CVFlags cv);


// set of declaration modifiers present
enum DeclFlags {
  DF_NONE        = 0x0000,

  // syntactic declaration modifiers
  DF_INLINE      = 0x0001,
  DF_VIRTUAL     = 0x0002,
  DF_FRIEND      = 0x0004,
  DF_MUTABLE     = 0x0008,
  DF_TYPEDEF     = 0x0010,
  DF_AUTO        = 0x0020,
  DF_REGISTER    = 0x0040,
  DF_STATIC      = 0x0080,
  DF_EXTERN      = 0x0100,

  // other stuff that's convenient for me
  DF_ENUMVAL     = 0x0200,    // not really a decl flag, but a Variable flag..
  DF_GLOBAL      = 0x0400,    // set for globals, unset for locals
  DF_INITIALIZED = 0x0800,    // true if has been declared with an initializer (or, for functions, with code)
  DF_BUILTIN     = 0x1000,    // true for e.g. __builtin_constant_p -- don't emit later

  ALL_DECLFLAGS  = 0x1FFF,
  NUM_DECLFLAGS  = 13
};

MLValue mlStorage(DeclFlags df);
extern char const * const declFlagNames[NUM_DECLFLAGS];      // 0="inline", 1="virtual", 2="friend", ..
string toString(DeclFlags df);


// --------------------- atomic types --------------------------
// ids for atomic types
typedef int AtomicTypeId;
enum { NULL_ATOMICTYPEID = -1 };


// C's built-in scalar types
enum SimpleTypeId {
  ST_CHAR,
  ST_UNSIGNED_CHAR,
  ST_SIGNED_CHAR,
  ST_BOOL,
  ST_INT,
  ST_UNSIGNED_INT,
  ST_LONG_INT,
  ST_UNSIGNED_LONG_INT,
  ST_LONG_LONG,                      // GNU extension
  ST_UNSIGNED_LONG_LONG,             // GNU extension
  ST_SHORT_INT,
  ST_UNSIGNED_SHORT_INT,
  ST_WCHAR_T,
  ST_FLOAT,
  ST_DOUBLE,
  ST_LONG_DOUBLE,
  ST_VOID,
  ST_ELLIPSIS,                       // used to encode vararg functions
  NUM_SIMPLE_TYPES,
  ST_BITMASK = 0xFF                  // for extraction for OR with CVFlags
};

// info about each simple type
struct SimpleTypeInfo {       
  char const *name;       // e.g. "unsigned char"
  int reprSize;           // # of bytes to store
  bool isInteger;         // ST_INT, etc., but not e.g. ST_FLOAT
};

bool isValid(SimpleTypeId id);                          // bounds check
SimpleTypeInfo const &simpleTypeInfo(SimpleTypeId id);

inline char const *simpleTypeName(SimpleTypeId id)
  { return simpleTypeInfo(id).name; }
inline int simpleTypeReprSize(SimpleTypeId id)
  { return simpleTypeInfo(id).reprSize; }
inline string toString(SimpleTypeId id)
  { return string(simpleTypeName(id)); }


// interface to types that are atomic in the sense that no
// modifiers can be stripped away; see types.txt
class AtomicType {
public:     // types
  enum Tag { T_SIMPLE, T_COMPOUND, T_ENUM, NUM_TAGS };

public:     // data
  // id as assigned by the global, flat type environment
  AtomicTypeId id;

public:     // funcs
  AtomicType();
  virtual ~AtomicType();

  virtual Tag getTag() const = 0;
  bool isSimpleType() const { return getTag() == T_SIMPLE; }
  bool isCompoundType() const { return getTag() == T_COMPOUND; }
  bool isEnumType() const { return getTag() == T_ENUM; }

  CAST_MEMBER_FN(SimpleType)
  CAST_MEMBER_FN(CompoundType)
  CAST_MEMBER_FN(EnumType)

  // this is type equality, *not* coercibility -- e.g. if
  // we say "extern type1 x" and then "extern type2 x" we
  // will allow it only if type1==type2
  bool equals(AtomicType const *obj) const;

  // print in C notation
  virtual string toCString() const = 0;

  // print in a Cil notation, using integer ids
  // for all references to other types
  virtual string toCilString(int depth=1) const = 0;

  // print in Cil with C notation in comments
  string toString(int depth=1) const;

  // return ML data structure; depth is the desired depth for chasing
  // named references (1 is a good choice, usually); cv is flags
  // applied from context, to be represented in the returned type
  virtual MLValue toMLValue(int depth, CVFlags cv) const = 0;

  // name of this type for references in Cil output
  virtual string uniqueName() const = 0;

  // size this type's representation occupies in memory
  virtual int reprSize() const = 0;

  ALLOC_STATS_DECLARE
};


// represents one of C's built-in types
class SimpleType : public AtomicType {
public:     // data
  SimpleTypeId type;

public:     // funcs
  SimpleType(SimpleTypeId t) : type(t) {}

  // also a default ctor because I make these in an array
  SimpleType() : type(NUM_SIMPLE_TYPES /*invalid*/) {}

  virtual Tag getTag() const { return T_SIMPLE; }
  virtual string toCString() const;
  virtual string toCilString(int depth) const;
  virtual MLValue toMLValue(int depth, CVFlags cv) const;
  virtual string uniqueName() const;
  virtual int reprSize() const;
};


// elements common to structs and enums
class NamedAtomicType : public AtomicType {
public:     // data
  string name;          // name of this struct or enum

public:
  NamedAtomicType(char const *name);
  ~NamedAtomicType();

  // globally unique name derived from 'name' and 'id'
  virtual string uniqueName() const;

  // the basic implementation
  virtual MLValue toMLValue(int depth, CVFlags cv) const;

  // the hook for the derived type to provide the complete info,
  // for use in a typedef
  virtual MLValue toMLContentsValue(int depth, CVFlags cv) const = 0;
};


// C++ class member access modes
enum AccessMode {
  AM_PUBLIC, AM_PROTECTED, AM_PRIVATE,
  NUM_ACCESS_MODES
};

// represent a user-defined compound type
class CompoundType : public NamedAtomicType {
public:     // types
  // NOTE: keep these consistent with TypeIntr (in file c.ast)
  enum Keyword { K_STRUCT, K_CLASS, K_UNION, NUM_KEYWORDS };

private:    // data
  Env *env;                   // (owner) members; NULL while only forward-declared

public:     // data
  Keyword const keyword;      // keyword used to introduce the type

public:     // funcs
  // create an incomplete (forward-declared) compound
  CompoundType(Keyword keyword, char const *name);
  ~CompoundType();

  // make it complete
  void makeComplete(Env *parentEnv, TypeEnv *te, VariableEnv *ve);

  bool isComplete() const { return env != NULL; }

  static char const *keywordName(Keyword k);

  virtual Tag getTag() const { return T_COMPOUND; }
  virtual string toCString() const;
  virtual string toCilString(int depth) const;
  virtual MLValue toMLContentsValue(int depth, CVFlags cv) const;
  virtual int reprSize() const;

  string toStringWithFields() const;
  string keywordAndName() const { return toCString(); }

  // the environment should only be directly accessed during
  // typeChecking, to avoid spreading dependency on this
  // implementation detail
  Env *getEnv() const { return env; }

  int numFields() const;
  Variable *getNthField(int index) const;
};


// represent an enumerated type
class EnumType : public NamedAtomicType {
public:     // data
  int nextValue;        // next value to assign to elements automatically

public:     // funcs
  EnumType(char const *n) : NamedAtomicType(n), nextValue(0) {}
  ~EnumType();

  virtual Tag getTag() const { return T_ENUM; }
  virtual string toCString() const;
  virtual string toCilString(int depth) const;
  virtual MLValue toMLContentsValue(int depth, CVFlags cv) const;
  virtual int reprSize() const;
};


// represent a single value in an enum
class EnumValue {
public:
  string name;              // the thing whose name is being defined
  EnumType const *type;     // enum in which it was declared
  int value;                // value it's assigned to

public:
  EnumValue(char const *n, EnumType const *t, int v);
  ~EnumValue();
};


// ------------------- constructed types -------------------------
// ids for types
typedef int TypeId;
enum { NULL_TYPEID = -1 };

// generic constructed type
class Type {
public:     // types
  enum Tag { T_ATOMIC, T_POINTER, T_FUNCTION, T_ARRAY };

public:     // data
  // id assigned by global type environment
  TypeId id;

private:    // funcs
  string idComment() const;

public:     // funcs
  Type();
  virtual ~Type();

  virtual Tag getTag() const = 0;
  bool isCVAtomicType() const { return getTag() == T_ATOMIC; }
  bool isPointerType() const { return getTag() == T_POINTER; }
  bool isFunctionType() const { return getTag() == T_FUNCTION; }
  bool isArrayType() const { return getTag() == T_ARRAY; }

  CAST_MEMBER_FN(CVAtomicType)
  CAST_MEMBER_FN(PointerType)
  CAST_MEMBER_FN(FunctionType)
  CAST_MEMBER_FN(ArrayType)

  // like above, this is equality, not coercibility
  bool equals(Type const *obj) const;

  // print the type, with an optional name like it was a declaration
  // for a variable of that type
  string toCString() const;
  string toCString(char const *name) const;

  // the left/right business is to allow us to print function
  // and array types in C's syntax
  virtual string leftString() const = 0;
  virtual string rightString() const;    // default: returns ""

  // same alternate syntaxes as AtomicType
  virtual string toCilString(int depth=1) const = 0;
  string toString(int depth=1) const;
  virtual MLValue toMLValue(int depth=1) const = 0;

  // size of representation
  virtual int reprSize() const = 0;

  // some common queries
  bool isSimpleType() const;
  SimpleType const &asSimpleTypeC() const;
  bool isSimple(SimpleTypeId id) const;
  bool isIntegerType() const;            // any of the simple integer types
  bool isUnionType() const;
  bool isVoid() const { return isSimple(ST_VOID); }

  ALLOC_STATS_DECLARE
};


// essentially just a wrapper around an atomic type, but
// also with optional const/volatile flags
class CVAtomicType : public Type {
public:
  AtomicType const *atomic;    // (serf) underlying type
  CVFlags cv;                  // const/volatile

private:    // funcs
  string atomicIdComment() const;

public:
  CVAtomicType(AtomicType const *a, CVFlags c)
    : atomic(a), cv(c) {}
  CVAtomicType(CVAtomicType const &obj)
    : DMEMB(atomic), DMEMB(cv) {}

  // just so I can make one static array of them...
  CVAtomicType() : atomic(NULL), cv(CV_NONE) {}

  bool innerEquals(CVAtomicType const *obj) const;

  virtual Tag getTag() const { return T_ATOMIC; }
  virtual string leftString() const;
  virtual string toCilString(int depth) const;
  virtual MLValue toMLValue(int depth=1) const;
  virtual int reprSize() const;
};


// "*" vs "&"
enum PtrOper {
  PO_POINTER, PO_REFERENCE
};

// type of a pointer or reference
class PointerType : public Type {
public:
  PtrOper op;                  // "*" or "&"
  CVFlags cv;                  // const/volatile, if "*"; refers to pointer *itself*
  Type const *atType;          // (serf) type of thing pointed-at

public:
  PointerType(PtrOper o, CVFlags c, Type const *a)
    : op(o), cv(c), atType(a) {}
  PointerType(PointerType const &obj)
    : DMEMB(op), DMEMB(cv), DMEMB(atType) {}

  bool innerEquals(PointerType const *obj) const;

  virtual Tag getTag() const { return T_POINTER; }
  virtual string leftString() const;
  virtual string rightString() const;
  virtual string toCilString(int depth) const;
  virtual MLValue toMLValue(int depth=1) const;
  virtual int reprSize() const;
};


// formal parameter to a function or function type
class Parameter {
public:
  string name;                 // can be "" to mean unnamed
  Type const *type;            // (serf) type of the parameter

  // todo: repr. of default arguments?

public:
  Parameter(Type const *t, char const *n="") 
    : name(n), type(t) {}
  ~Parameter();

  string toString() const;
};


// type of a function
class FunctionType : public Type {
public:
  Type const *retType;         // (serf) type of return value
  CVFlags cv;                  // const/volatile for class member fns
  ObjList<Parameter> params;   // list of function parameters
  bool acceptsVarargs;         // true if add'l args are allowed

public:
  FunctionType(Type const *retType, CVFlags cv);
  virtual ~FunctionType();

  bool innerEquals(FunctionType const *obj) const;

  // append a parameter to the parameters list
  void addParam(Parameter *param);

  virtual Tag getTag() const { return T_FUNCTION; }
  virtual string leftString() const;
  virtual string rightString() const;
  virtual string toCilString(int depth) const;
  virtual MLValue toMLValue(int depth=1) const;
  virtual int reprSize() const;
};


// type of an array
class ArrayType : public Type {
public:
  Type const *eltType;         // (serf) type of the elements
  bool hasSize;                // true if a size is specified
  int size;                    // specified size, if any

public:
  ArrayType(Type const *e, int s)
    : eltType(e), hasSize(true), size(s) {}
  ArrayType(Type const *e)
    : eltType(e), hasSize(false), size(-1) {}

  bool innerEquals(ArrayType const *obj) const;

  virtual Tag getTag() const { return T_ARRAY; }
  virtual string leftString() const;
  virtual string rightString() const;
  virtual string toCilString(int depth) const;
  virtual MLValue toMLValue(int depth=1) const;
  virtual int reprSize() const;
};


#endif // __CC_TYPE_H
