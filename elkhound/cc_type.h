// cc_type.h
// compile-type representation of C++ types
// see types.txt

#ifndef CC_TYPE_H
#define CC_TYPE_H

#include "str.h"          // string
#include "objlist.h"      // ObjList
//#include "mlvalue.h"      // MLValue
#include "cc_flags.h"     // CVFlags, DeclFlags, SimpleTypeId
#include "strtable.h"     // StringRef
#include "strsobjdict.h"  // StrSObjDict

// below, the type language refers to the AST language in exactly
// one place: function pre/post conditions; the type language treats
// these opaquely; it is important to prevent the type language from
// depending on the AST language
class FA_precondition;    // c.ast
class FA_postcondition;

// fwd in this file
class SimpleType;
class CompoundType;
class EnumType;
class CVAtomicType;
class PointerType;
class FunctionType;
class ArrayType;
class Type;

//MLValue mlStorage(DeclFlags df);

// static data consistency checker
void cc_type_checker();

// --------------------- atomic types --------------------------
// ids for atomic types
//typedef int AtomicTypeId;
//enum { NULL_ATOMICTYPEID = -1 };


// interface to types that are atomic in the sense that no
// modifiers can be stripped away; see types.txt
class AtomicType {
public:     // types
  enum Tag { T_SIMPLE, T_COMPOUND, T_ENUM, NUM_TAGS };

public:     // data
  // id unique across all atomic types
  //AtomicTypeId id;

public:     // funcs
  AtomicType();
  virtual ~AtomicType();
                    
  // stand-in if I'm not really using ids..
  int getId() const { return (int)this; }

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
  //virtual MLValue toMLValue(int depth, CVFlags cv) const = 0;

  // name of this type for references in Cil output
  virtual string uniqueName() const = 0;

  // size this type's representation occupies in memory
  virtual int reprSize() const = 0;

  ALLOC_STATS_DECLARE
};


// represents one of C's built-in types;
// there are exactly as many of these objects as there are built-in types
class SimpleType : public AtomicType {
public:     // data
  SimpleTypeId type;

  // global read-only array for each built-in type
  static SimpleType const fixed[NUM_SIMPLE_TYPES];

public:     // funcs
  SimpleType(SimpleTypeId t) : type(t) {}

  // also a default ctor because I make these in an array
  //SimpleType() : type(NUM_SIMPLE_TYPES /*invalid*/) {}

  virtual Tag getTag() const { return T_SIMPLE; }
  virtual string toCString() const;
  virtual string toCilString(int depth) const;
  //virtual MLValue toMLValue(int depth, CVFlags cv) const;
  virtual string uniqueName() const;
  virtual int reprSize() const;
};


// elements common to structs and enums
class NamedAtomicType : public AtomicType {
public:     // data
  StringRef name;          // (nullable) user-assigned name of this struct or enum

public:
  NamedAtomicType(StringRef name);
  ~NamedAtomicType();

  // globally unique name derived from 'name' and 'id'
  virtual string uniqueName() const;

  // the basic implementation
  //virtual MLValue toMLValue(int depth, CVFlags cv) const;

  // the hook for the derived type to provide the complete info,
  // for use in a typedef
  //virtual MLValue toMLContentsValue(int depth, CVFlags cv) const = 0;
};


// C++ class member access modes
enum AccessMode {
  AM_PUBLIC, 
  AM_PROTECTED, 
  AM_PRIVATE,
  NUM_ACCESS_MODES
};

// represent a user-defined compound type
class CompoundType : public NamedAtomicType {
public:      // types
  // NOTE: keep these consistent with TypeIntr (in file c.ast)
  enum Keyword { K_STRUCT, K_CLASS, K_UNION, NUM_KEYWORDS };

  // one of these for each field in the struct
  class Field {
  public:
    StringRef name;
    Type const *type;

  public:
    Field(StringRef n, Type const *t) : name(n), type(t) {}
  };

private:     // data
  ObjList<Field> fields;               // fields in this type
  StringSObjDict<Field> fieldIndex;    // dictionary for name lookup

public:      // data
  bool forward;               // true when it's only fwd-declared
  Keyword const keyword;      // keyword used to introduce the type

public:      // funcs
  // create an incomplete (forward-declared) compound
  CompoundType(Keyword keyword, StringRef name);
  ~CompoundType();

  // make it complete
  //void makeComplete(Env *parentEnv, TypeEnv *te, VariableEnv *ve);

  bool isComplete() const { return !forward; }

  static char const *keywordName(Keyword k);

  virtual Tag getTag() const { return T_COMPOUND; }
  virtual string toCString() const;
  virtual string toCilString(int depth) const;
  //virtual MLValue toMLContentsValue(int depth, CVFlags cv) const;
  virtual int reprSize() const;

  string toStringWithFields() const;
  string keywordAndName() const { return toCString(); }

  // the environment should only be directly accessed during
  // typeChecking, to avoid spreading dependency on this
  // implementation detail
  //Env *getEnv() const { return env; }

  int numFields() const;
  Field const *getNthField(int index) const;
  Field const *getNamedField(StringRef name) const;
  
  Field *addField(StringRef name, Type const *type);
};


// represent an enumerated type
class EnumType : public NamedAtomicType {
public:     // types
  // represent a single value in an enum
  class Value {
  public:
    StringRef name;           // the thing whose name is being defined
    EnumType const *type;     // enum in which it was declared
    int value;                // value it's assigned to

  public:
    Value(StringRef n, EnumType const *t, int v);
    ~Value();
  };

public:     // data
  ObjList<Value> values;              // values in this enumeration
  StringSObjDict<Value> valueIndex;   // name-based lookup
  int nextValue;                      // next value to assign to elements automatically

public:     // funcs
  EnumType(StringRef n) : NamedAtomicType(n), nextValue(0) {}
  ~EnumType();

  virtual Tag getTag() const { return T_ENUM; }
  virtual string toCString() const;
  virtual string toCilString(int depth) const;
  //virtual MLValue toMLContentsValue(int depth, CVFlags cv) const;
  virtual int reprSize() const;
  
  Value *addValue(StringRef name, int value);
  Value const *getValue(StringRef name) const;
};


// ------------------- constructed types -------------------------
// ids for types
//typedef int TypeId;
//enum { NULL_TYPEID = -1 };

// generic constructed type
class Type {
public:     // types
  enum Tag { T_ATOMIC, T_POINTER, T_FUNCTION, T_ARRAY };

public:     // data
  // this id doesn't mean all that much; in particular, there is
  // *no* guarantee that equal types have the same id
  //TypeId id;

private:    // funcs
  string idComment() const;

public:     // funcs
  Type();
  virtual ~Type();

  int getId() const { return (int)this; }

  virtual Tag getTag() const = 0;
  bool isCVAtomicType() const { return getTag() == T_ATOMIC; }
  bool isPointerType() const { return getTag() == T_POINTER; }
  bool isFunctionType() const { return getTag() == T_FUNCTION; }
  bool isArrayType() const { return getTag() == T_ARRAY; }

  CAST_MEMBER_FN(CVAtomicType)
  CAST_MEMBER_FN(PointerType)
  CAST_MEMBER_FN(FunctionType)
  CAST_MEMBER_FN(ArrayType)

  // like above, this is (structural) equality, not coercibility;
  // internally, this calls the innerEquals() method on the two
  // objects, once their tags have been established to be equal
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
  //virtual MLValue toMLValue(int depth=1) const = 0;

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
public:     // data
  AtomicType const *atomic;    // (serf) underlying type
  CVFlags cv;                  // const/volatile
                    
  // global read-only array of non-const, non-volatile built-ins
  static CVAtomicType const fixed[NUM_SIMPLE_TYPES];

private:    // funcs
  string atomicIdComment() const;

public:     // funcs
  CVAtomicType(AtomicType const *a, CVFlags c)
    : atomic(a), cv(c) {}
  CVAtomicType(CVAtomicType const &obj)
    : DMEMB(atomic), DMEMB(cv) {}

  // just so I can make one static array of built-ins w/o const/volatile
  //CVAtomicType() : atomic(NULL), cv(CV_NONE) {}

  bool innerEquals(CVAtomicType const *obj) const;

  virtual Tag getTag() const { return T_ATOMIC; }
  virtual string leftString() const;
  virtual string toCilString(int depth) const;
  //virtual MLValue toMLValue(int depth=1) const;
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
  //virtual MLValue toMLValue(int depth=1) const;
  virtual int reprSize() const;
};


// type of a function
class FunctionType : public Type {
public:     // types
  // formal parameter to a function or function type
  class Param {
  public:
    StringRef name;              // can be NULL to mean unnamed
    Type const *type;            // (serf) type of the parameter

  public:
    Param(StringRef n, Type const *t)
      : name(n), type(t) {}
    ~Param();

    string toString() const;
  };

public:     // data
  Type const *retType;         // (serf) type of return value
  //CVFlags cv;                  // const/volatile for class member fns
  ObjList<Param> params;       // list of function parameters
  bool acceptsVarargs;         // true if add'l args are allowed
  
  // thmprv extensions
  FA_precondition *precondition;     // (serf) precondition predicate
  FA_postcondition *postcondition;   // (serf) postcondition predicate

public:     // funcs
  FunctionType(Type const *retType/*, CVFlags cv*/);
  virtual ~FunctionType();

  bool innerEquals(FunctionType const *obj) const;

  // append a parameter to the parameters list
  void addParam(Param *param);

  virtual Tag getTag() const { return T_FUNCTION; }
  virtual string leftString() const;
  virtual string rightString() const;
  virtual string toCilString(int depth) const;
  //virtual MLValue toMLValue(int depth=1) const;
  virtual int reprSize() const;
};


// type of an array
class ArrayType : public Type {
public:
  Type const *eltType;         // (serf) type of the elements
  bool hasSize;                // true if a size is specified
  int size;                    // specified size, if 'hasSize'

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
  //virtual MLValue toMLValue(int depth=1) const;
  virtual int reprSize() const;
};


#endif // CC_TYPE_H
