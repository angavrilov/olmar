// cc_type.h
// compile-type representation of C++ types
// see types.txt

#ifndef __CC_TYPE_H
#define __CC_TYPE_H

#include "str.h"        // string
#include "objlist.h"    // ObjList

class Env;              // cc_env.h


// --------------------- atomic types --------------------------
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
  NUM_SIMPLE_TYPES
};

bool isValid(SimpleTypeId id);                     // bounds check
char const *simpleTypeName(SimpleTypeId id);       // e.g. "unsigned char"
int simpleTypeReprSize(SimpleTypeId id);           // size of representation


// interface to types that are atomic in the sense that no
// modifiers can be stripped away; see types.txt
class AtomicType {
public:     // types
  enum Tag { T_SIMPLE, T_COMPOUND, T_ENUM, NUM_TAGS };

public:     // funcs
  virtual ~AtomicType();

  virtual Tag getTag() const = 0;
  bool isSimple() const { return getTag() == T_SIMPLE; }
  bool isCompound() const { return getTag() == T_COMPOUND; }
  bool isEnum() const { return getTag() == T_ENUM; }
  
  virtual string toString() const = 0;
  
  // size this type's representation occupies in memory
  virtual int reprSize() const = 0;
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
  virtual string toString() const;
  virtual int reprSize() const;
};


// C++ class member access modes
enum AccessMode {
  AM_PUBLIC, AM_PROTECTED, AM_PRIVATE,
  NUM_ACCESS_MODES
};

// represent a user-defined compound type
class CompoundType : public AtomicType {
public:     // types
  enum Keyword { K_STRUCT, K_CLASS, K_UNION, NUM_KEYWORDS };

public:     // data
  Keyword keyword;      // keyword used to introduce the type
  string name;          // name of this compound
  Env *env;             // (owner) members; NULL while only forward-declared

public:     // funcs
  // create an incomplete (forward-declared) compound
  CompoundType(Keyword keyword, char const *name);
  ~CompoundType();

  // make it complete
  void makeComplete(Env *parentEnv);

  bool isComplete() const { return env != NULL; }

  static char const *keywordName(Keyword k);

  virtual Tag getTag() const { return T_COMPOUND; }
  virtual string toString() const;
  virtual int reprSize() const;

  string toStringWithFields() const;
};


// represent an enumerated type
class EnumType : public AtomicType {
public:
  string name;          // name of this enum

  // todo: represent enumerated elements

public:
  EnumType(char const *n) : name(n) {}
  ~EnumType();

  virtual Tag getTag() const { return T_ENUM; }
  virtual string toString() const;
  virtual int reprSize() const;
};


// ------------------- constructed types -------------------------
class CVAtomicType;
class PointerType;
class FunctionType;
class ArrayType;

// generic constructed type
class Type {
public:     // types
  enum Tag { T_ATOMIC, T_POINTER, T_FUNCTION, T_ARRAY };

public:     // funcs
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

  // print the type, with an optional name like it was a declaration
  // for a variable of that type
  string toString() const;
  string toString(char const *name) const;

  // the left/right business is to allow us to print function
  // and array types in C's syntax
  virtual string leftString() const = 0;
  virtual string rightString() const;    // default: returns ""
  
  // size of representation
  virtual int reprSize() const = 0;
};


// set: which of "const" and/or "volatile" is specified
enum CVFlags {
  CV_NONE     = 0,
  CV_CONST    = 1,
  CV_VOLATILE = 2,
  CV_OWNER    = 4,
  CV_ALL      = 7
};

// essentially just a wrapper around an atomic type, but
// also with optional const/volatile flags
class CVAtomicType : public Type {
public:
  AtomicType const *atomic;    // (serf) underlying type
  CVFlags cv;                  // const/volatile

public:
  CVAtomicType(AtomicType const *a, CVFlags c)
    : atomic(a), cv(c) {}
  CVAtomicType(CVAtomicType const &obj)
    : DMEMB(atomic), DMEMB(cv) {}

  // just so I can make one static array of them...
  CVAtomicType() : atomic(NULL), cv(CV_NONE) {}

  virtual Tag getTag() const { return T_ATOMIC; }
  virtual string leftString() const;
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

  virtual Tag getTag() const { return T_POINTER; }
  virtual string leftString() const;
  virtual string rightString() const;
  virtual int reprSize() const;
};


// formal parameter to a function or function type
class Parameter {
public:
  string name;                 // can be "" to mean unnamed
  Type const *type;            // (serf) type of the parameter

  // todo: repr. of default arguments?

public:
  Parameter(Type const *t) : name(""), type(t) {}
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

  // append a parameter to the parameters list
  void addParam(Parameter *param);

  virtual Tag getTag() const { return T_FUNCTION; }
  virtual string leftString() const;
  virtual string rightString() const;
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

  virtual Tag getTag() const { return T_ARRAY; }
  virtual string leftString() const;
  virtual string rightString() const;
  virtual int reprSize() const;
};


#endif // __CC_TYPE_H
