// cc_type.h            see license.txt for copyright and terms of use
// compile-type representation of C++ types
// see types.txt

#ifndef CC_TYPE_H
#define CC_TYPE_H

#include "str.h"          // string
#include "objlist.h"      // ObjList
#include "cc_flags.h"     // CVFlags, DeclFlags, SimpleTypeId
#include "strtable.h"     // StringRef
#include "strsobjdict.h"  // StrSObjDict

class Variable;           // variable.h

// fwd in this file
class SimpleType;
class CompoundType;
class EnumType;
class CVAtomicType;
class PointerType;
class FunctionType;
class ArrayType;
class Type;

// static data consistency checker
void cc_type_checker();

// --------------------- atomic types --------------------------
// interface to types that are atomic in the sense that no
// modifiers can be stripped away; see types.txt
class AtomicType {
public:     // types
  enum Tag { T_SIMPLE, T_COMPOUND, T_ENUM, NUM_TAGS };

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

  virtual Tag getTag() const { return T_SIMPLE; }
  virtual string toCString() const;
  virtual string toCilString(int depth) const;
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
    StringRef name;                  // programmer-given name
    int const index;                 // first field is 0, next is 1, etc.
    Type const *type;                // declared field type
    CompoundType const *compound;    // (serf) compound in which this appears

    // I include a pointer to the introduction; since I want
    // to keep the type language independent of the AST language, I
    // will continue to store redundant info, regarding 'decl' as
    // something which might go away at some point
    Variable *decl;                  // (nullable serf)

  public:
    Field(StringRef n, int i, Type const *t, CompoundType const *c, Variable *d)
      : name(n), index(i), type(t), compound(c), decl(d) {}
  };

private:     // data
  ObjList<Field> fields;               // fields in this type
  StringSObjDict<Field> fieldIndex;    // dictionary for name lookup
  int fieldCounter;                    // # of fields

public:      // data
  bool forward;               // true when it's only fwd-declared
  Keyword const keyword;      // keyword used to introduce the type

public:      // funcs
  // create an incomplete (forward-declared) compound
  CompoundType(Keyword keyword, StringRef name);
  ~CompoundType();

  bool isComplete() const { return !forward; }
  bool nunFields() const { return fieldCounter; }

  static char const *keywordName(Keyword k);

  virtual Tag getTag() const { return T_COMPOUND; }
  virtual string toCString() const;
  virtual string toCilString(int depth) const;
  virtual int reprSize() const;

  string toStringWithFields() const;
  string keywordAndName() const { return toCString(); }

  int numFields() const;
  Field const *getNthField(int index) const;         // must exist
  Field const *getNamedField(StringRef name) const;  // returns NULL if doesn't exist

  Field *addField(StringRef name, Type const *type, 
                  /*nullable*/ Variable *d);
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

    // similar to fields, I keep a record of where this came from
    Variable *decl;           // (nullable serf)

  public:
    Value(StringRef n, EnumType const *t, int v, Variable *d);
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
  virtual int reprSize() const;

  Value *addValue(StringRef name, int value, /*nullable*/ Variable *d);
  Value const *getValue(StringRef name) const;
};


// ------------------- constructed types -------------------------
// generic constructed type
class Type {
public:     // types
  enum Tag { T_ATOMIC, T_POINTER, T_FUNCTION, T_ARRAY };

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

  // size of representation
  virtual int reprSize() const = 0;

  // some common queries
  bool isSimpleType() const;
  SimpleType const &asSimpleTypeC() const;
  bool isSimple(SimpleTypeId id) const;
  bool isIntegerType() const;            // any of the simple integer types
  bool isUnionType() const { return isCompoundTypeOf(CompoundType::K_UNION); }
  bool isStructType() const { return isCompoundTypeOf(CompoundType::K_STRUCT); }
  bool isCompoundTypeOf(CompoundType::Keyword keyword) const;
  bool isVoid() const { return isSimple(ST_VOID); }
  bool isError() const { return isSimple(ST_ERROR); }
  CompoundType const *ifCompoundType() const;     // NULL or corresp. compound
  bool isOwnerPtr() const;

  // pointer/reference stuff
  bool isPointer() const;                // as opposed to reference or non-pointer
  bool isReference() const;
  bool isLval() const { return isReference(); }    // C terminology
  Type const *asRval() const;            // if I am a reference, return referrent type

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

  bool innerEquals(CVAtomicType const *obj) const;

  virtual Tag getTag() const { return T_ATOMIC; }
  virtual string leftString() const;
  virtual string toCilString(int depth) const;
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
  PointerType(PtrOper o, CVFlags c, Type const *a);
  PointerType(PointerType const &obj)
    : DMEMB(op), DMEMB(cv), DMEMB(atType) {}

  bool innerEquals(PointerType const *obj) const;

  virtual Tag getTag() const { return T_POINTER; }
  virtual string leftString() const;
  virtual string rightString() const;
  virtual string toCilString(int depth) const;
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

    // syntactic introduction
    Variable *decl;              // (serf)

  public:
    Param(StringRef n, Type const *t, Variable *d)
      : name(n), type(t), decl(d) {}
    ~Param();

    string toString() const;
  };

public:     // data
  Type const *retType;         // (serf) type of return value
  CVFlags cv;                  // const/volatile for class member fns
  ObjList<Param> params;       // list of function parameters
  bool acceptsVarargs;         // true if add'l args are allowed

public:     // funcs
  FunctionType(Type const *retType, CVFlags cv);
  virtual ~FunctionType();

  bool innerEquals(FunctionType const *obj) const;

  // append a parameter to the parameters list
  void addParam(Param *param);

  virtual Tag getTag() const { return T_FUNCTION; }
  virtual string leftString() const;
  virtual string rightString() const;
  virtual string toCilString(int depth) const;
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
  virtual int reprSize() const;
};


//--------------- lots of useful type constructors ---------------
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
inline Type const *makePtrType(Type const *type)
  { return makePtrOperType(PO_POINTER, CV_NONE, type); }
inline Type const *makeRefType(Type const *type)
  { return makePtrOperType(PO_REFERENCE, CV_NONE, type); }

// map a simple type into its CVAtomicType (with no const or
// volatile) representative
CVAtomicType const *getSimpleType(SimpleTypeId st);


#endif // CC_TYPE_H
