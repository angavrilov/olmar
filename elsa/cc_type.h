// cc_type.h            see license.txt for copyright and terms of use
// compile-type representation of C++ types
// see cc_type.html

// The original design intent was that type representation would be
// completely independent of the Expression language, or anything
// else from cc.ast.  That is, types should have meaning independent
// of the language of values, or of the syntax used to describe them
// in source code.
//
// However, the practicalities of parsing C++ have forced my hand in a
// couple of places.  Those places are marked with the annotation
// "(AST pointer)".  In all cases so far, I can forsee an intermediate
// transformation which would remove (e.g. nullify) all the AST
// pointers in the process of creating a "minimized" C++ subset.  In
// particular, it should be possible to translate away everything
// related to templates.
//
// *Please* do not add any additional AST pointers unless it is
// really necessary.  Also please make sure all the types can
// intelligbly print themselves *without* inspecting the AST pointers
// (by storing a textual representation if necessary).

#ifndef CC_TYPE_H
#define CC_TYPE_H

#include "str.h"          // string
#include "objlist.h"      // ObjList
#include "sobjlist.h"     // SObjList
#include "cc_flags.h"     // CVFlags, DeclFlags, SimpleTypeId
#include "strtable.h"     // StringRef
#include "strsobjdict.h"  // StringSObjDict
#include "strobjdict.h"   // StringObjDict
#include "cc_scope.h"     // Scope
#include "fakelist.h"     // FakeList
#include "srcloc.h"       // SourceLoc

class Variable;           // variable.h
class Env;                // cc_env.h
class TS_classSpec;       // cc.ast
class Expression;         // cc.ast
class TemplateArgument;   // cc.ast
class D_pointer;          // cc.ast
class D_func;             // cc.ast
class D_ptrToMember;      // cc.ast
class TypeSpecifier;      // cc.ast

// fwd in this file
class SimpleType;
class CompoundType;
class BaseClass;
class EnumType;
class TypeVariable;
class CVAtomicType;
class PointerType;
class FunctionType;
class ArrayType;
class PointerToMemberType;
class Type;
class TemplateParams;
class STemplateArgument;
class ClassTemplateInfo;
class TypeFactory;
class BasicTypeFactory;

// static data consistency checker
void cc_type_checker();


// --------------------- atomic types --------------------------
// interface to types that are atomic in the sense that no
// modifiers can be stripped away; see types.txt
class AtomicType {
public:     // types
  enum Tag { T_SIMPLE, T_COMPOUND, T_ENUM, T_TYPEVAR, NUM_TAGS };

public:     // funcs
  AtomicType();
  virtual ~AtomicType();

  // stand-in if I'm not really using ids..
  int getId() const { return (int)this; }

  virtual Tag getTag() const = 0;
  bool isSimpleType() const   { return getTag() == T_SIMPLE; }
  bool isCompoundType() const { return getTag() == T_COMPOUND; }
  bool isEnumType() const     { return getTag() == T_ENUM; }
  bool isTypeVariable() const { return getTag() == T_TYPEVAR; }

  DOWNCAST_FN(SimpleType)
  DOWNCAST_FN(CompoundType)
  DOWNCAST_FN(EnumType)
  DOWNCAST_FN(TypeVariable)

  // this is type equality, *not* coercibility -- e.g. if
  // we say "extern type1 x" and then "extern type2 x" we
  // will allow it only if type1==type2
  bool equals(AtomicType const *obj) const;

  // print in C notation
  virtual string toCString() const = 0;
  string toString() const { return toCString(); }

  // size this type's representation occupies in memory
  virtual int reprSize() const = 0;

  ALLOC_STATS_DECLARE
};


// represents one of C's built-in types;
// there are exactly as many of these objects as there are built-in types
class SimpleType : public AtomicType {
public:     // data
  SimpleTypeId const type;

  // global read-only array for each built-in type        
  // (read-only is enforced by making all of SimpleType's members
  // const, rather then the objects themselves, because doing the
  // latter would clash AtomicType pointers not being const below)
  static SimpleType fixed[NUM_SIMPLE_TYPES];

public:     // funcs
  SimpleType(SimpleTypeId t) : type(t) {}

  // AtomicType interface
  virtual Tag getTag() const { return T_SIMPLE; }
  virtual string toCString() const;
  virtual int reprSize() const;
};


// elements common to structs and enums
class NamedAtomicType : public AtomicType {
public:     // data
  StringRef name;          // (nullable) user-assigned name of this struct or enum
  Variable *typedefVar;    // (owner) implicit typedef variable
  AccessKeyword access;    // accessibility of this type in its declaration context

public:
  NamedAtomicType(StringRef name);
  ~NamedAtomicType();
};


// represent a base class
class BaseClass {
public:
  CompoundType *ct;          // (serf) the base class itself
  AccessKeyword access;      // access mode of the inheritance
  bool isVirtual;            // true for virtual inheritance

public:
  BaseClass(BaseClass const &obj)
    : DMEMB(ct), DMEMB(access), DMEMB(isVirtual) {}
  BaseClass(CompoundType *c, AccessKeyword a, bool v)
    : ct(c), access(a), isVirtual(v) {}
};

// represent an instance of a base class in a particular class'
// inheritance hierarchy; it is these things that represent things
// like diamond inheritance patterns; the terminology comes from
// [cppstd 10.1 para 4]
class BaseClassSubobj : public BaseClass {
public:
  // classes that this one inherits from; non-owning since the
  // 'parents' links form a DAG, not a tree; in fact I regard
  // this list as an owner of exactly those sub-objects whose
  // inheritance is *not* virtual
  SObjList<BaseClassSubobj> parents;

  // for visit-once-only traversals
  mutable bool visited;

public:
  BaseClassSubobj(BaseClass const &base);
  ~BaseClassSubobj();
                           
  // name and virtual address to uniquely identify this object
  string canonName() const;
};

// represent a user-defined compound type; the members of the
// compound are whatever has been entered in the Scope
class CompoundType : public NamedAtomicType, public Scope {
public:      // types
  // NOTE: keep these consistent with TypeIntr (in file cc_flags.h)
  enum Keyword { K_STRUCT, K_CLASS, K_UNION, NUM_KEYWORDS };

public:      // data
  bool forward;               // true when it's only fwd-declared
  Keyword keyword;            // keyword used to introduce the type

  // nonstatic data members, in the order they appear within the body
  // of the class definition; note that this is partially redundant
  // with the Scope's 'variables' map, and hence should not be changed
  // without also updating that map
  SObjList<Variable> dataMembers;

  // classes from which this one inherits; 'const' so you have to
  // use 'addBaseClass', but public to make traversal easy
  const ObjList<BaseClass> bases;

  // collected virtual base class subobjects
  ObjList<BaseClassSubobj> virtualBases;

  // this is the root of the subobject hierarchy diagram
  // invariant: subobj.ct == this
  BaseClassSubobj subobj;

  // for template classes, this is the list of template parameters,
  // and a list of already-instantiated versions
  ClassTemplateInfo *templateInfo;    // (owner)

  // if this class is a template instantiation, 'instName' is
  // the class name plus a rendering of the arguments; otherwise
  // it's the same as 'name'; this is for debugging
  StringRef instName;

  // AST node that describes this class; used for implementing
  // templates (AST pointer)
  TS_classSpec *syntax;               // (serf)

private:     // funcs
  void makeSubobjHierarchy(BaseClassSubobj *subobj, BaseClass const *newBase);

  BaseClassSubobj const *findVirtualSubobjectC(CompoundType const *ct) const;
  BaseClassSubobj *findVirtualSubobject(CompoundType *ct)
    { return const_cast<BaseClassSubobj*>(findVirtualSubobjectC(ct)); }

  static void clearVisited_helper(BaseClassSubobj const *subobj);

  static void getSubobjects_helper
    (SObjList<BaseClassSubobj const> &dest, BaseClassSubobj const *subobj);

protected:   // funcs
  // create an incomplete (forward-declared) compound
  // experiment: force creation of these to go through the factory too
  CompoundType(Keyword keyword, StringRef name);
  friend class TypeFactory;
  
  // override no-op implementation in Scope
  virtual void afterAddVariable(Variable *v);

public:      // funcs
  virtual ~CompoundType();

  bool isComplete() const { return !forward; }
  
  // true if this is a class that is incomplete because it requires
  // template arguments to be supplied (i.e. not true for classes
  // that come from templates, but all arguments have been supplied)
  bool isTemplate() const;

  static char const *keywordName(Keyword k);

  // AtomicType interface
  virtual Tag getTag() const { return T_COMPOUND; }
  virtual string toCString() const;
  virtual int reprSize() const;

  string toStringWithFields() const;
  string keywordAndName() const { return toCString(); }

  int numFields() const;

  // returns NULL if doesn't exist
  Variable const *getNamedFieldC(StringRef name, Env &env, LookupFlags f=LF_NONE) const
    { return lookupVariableC(name, env, f); }
  Variable *getNamedField(StringRef name, Env &env, LookupFlags f=LF_NONE)
    { return lookupVariable(name, env, f); }

  // alias for Scope::addVariable (should probably be deleted)
  void addField(Variable *v);

  // sm: for compatibility with existing code
  SObjList<Variable> &get_data_variables_in_order() 
    { return dataMembers; }

  // return the ordinal position of the named nonstatic data field
  // in this class, starting with 0; or, return -1 if the field does
  // not exist (this is a query on 'dataMembers')
  int getDataMemberPosition(StringRef name) const;

  // add to 'bases'; incrementally maintains 'virtualBases'
  void addBaseClass(BaseClass * /*owner*/ newBase);

  // true if this class inherits from 'ct', either directly or
  // indirectly, and the inheritance is virtual
  bool hasVirtualBase(CompoundType const *ct) const
    { return !!findVirtualSubobjectC(ct); }

  // set all the 'visited' fields to false in the subobject hierarchy
  void clearSubobjVisited() const;

  // collect all of the subobjects into a list; each subobject
  // appears exactly once
  void getSubobjects(SObjList<BaseClassSubobj const> &dest) const;

  // render the subobject hierarchy to a 'dot' graph
  string renderSubobjHierarchy() const;

  // how many times does 'ct' appear as a subobject?
  // returns 1 if ct==this
  int countBaseClassSubobjects(CompoundType const *ct) const;
                                                            
  bool hasUnambiguousBaseClass(CompoundType const *ct) const
    { return countBaseClassSubobjects(ct)==1; }
  bool hasBaseClass(CompoundType const *ct) const
    { return countBaseClassSubobjects(ct)>=1; }
};

string toString(CompoundType::Keyword k);


// represent an enumerated type
class EnumType : public NamedAtomicType {
public:     // types
  // represent a single value in an enum
  class Value {
  public:
    StringRef name;           // the thing whose name is being defined
    EnumType *type;           // enum in which it was declared
    int value;                // value it's assigned to

    // similar to fields, I keep a record of where this came from
    Variable *decl;           // (nullable serf)

  public:
    Value(StringRef n, EnumType *t, int v, Variable *d);
    ~Value();
  };

public:     // data
  ObjList<Value> values;              // values in this enumeration
  StringSObjDict<Value> valueIndex;   // name-based lookup
  int nextValue;                      // next value to assign to elements automatically

public:     // funcs
  EnumType(StringRef n) : NamedAtomicType(n), nextValue(0) {}
  ~EnumType();

  // AtomicType interface
  virtual Tag getTag() const { return T_ENUM; }
  virtual string toCString() const;
  virtual int reprSize() const;

  Value *addValue(StringRef name, int value, /*nullable*/ Variable *d);
  Value const *getValue(StringRef name) const;
};


// ------------------- constructed types -------------------------
// generic constructed type; to allow client analyses to annotate the
// description of types, this class becomes is inherited by "Type",
// the class that all of the rest of the parser regards as being a
// "type"
class BaseType {
public:     // types
  enum Tag { T_ATOMIC, T_POINTER, T_FUNCTION, T_ARRAY, T_POINTERTOMEMBER };
  
  // moved this declaration into Type, along with the declaration of
  // 'anyCtorSatisfies', so as not to leak the name "BaseType"
  //typedef bool (*TypePred)(Type const *t);

private:    // funcs
  string idComment() const;

private:    // disallowed
  BaseType(BaseType&);

protected:  // funcs
  // the constructor of BaseType is protected because I want to force
  // all object creation to go through the factory (below)
  BaseType();

public:     // funcs
  virtual ~BaseType();

  virtual Tag getTag() const = 0;
  bool isCVAtomicType() const { return getTag() == T_ATOMIC; }
  bool isPointerType() const { return getTag() == T_POINTER; }
  bool isFunctionType() const { return getTag() == T_FUNCTION; }
  bool isArrayType() const { return getTag() == T_ARRAY; }
  bool isPointerToMemberType() const { return getTag() == T_POINTERTOMEMBER; }

  DOWNCAST_FN(CVAtomicType)
  DOWNCAST_FN(PointerType)
  DOWNCAST_FN(FunctionType)
  DOWNCAST_FN(ArrayType)
  DOWNCAST_FN(PointerToMemberType)

  // flags to control how "equal" the types must be
  enum EqFlags {
    EF_EXACT           = 0x00,     // complete equality

    EF_SIGNATURE       = 0x01,     // function signatures equivalence
    EF_IGNORE_TOP_CV   = 0x02,     // ok if toplevel cv flags differ
    EF_SKIPTHIS        = 0x04,     // skip any 'this' params at top level

    EF_ALL             = 0x07,     // all flags set to 1

    // this is the set of flags that automatically propagate down
    // the type tree equality checker; others are suppressed once
    // the first type constructor looks at them
    EF_PROP            = EF_SIGNATURE
  };

  // like above, this is (structural) equality, not coercibility;
  // internally, this calls the innerEquals() method on the two
  // objects, once their tags have been established to be equal
  bool equals(BaseType const *obj, EqFlags flags = EF_EXACT) const;

  // print the type, with an optional name like it was a declaration
  // for a variable of that type
  string toCString() const;
  string toCString(char const *name) const;
  string toString() const { return toCString(); }

  // the left/right business is to allow us to print function
  // and array types in C's syntax; if 'innerParen' is true then
  // the topmost type constructor should print the inner set of
  // paretheses
  virtual string leftString(bool innerParen=true) const = 0;
  virtual string rightString(bool innerParen=true) const;    // default: returns ""

  // size of representation
  virtual int reprSize() const = 0;

  // filter on all type constructors (i.e. non-atomic types); return
  // true if any constructor satisfies 'pred'
  //virtual bool anyCtorSatisfies(TypePred pred) const=0;

  // return the cv flags that apply to this type, if any;
  // default returns CV_NONE
  virtual CVFlags getCVFlags() const;
  bool isConst() const { return !!(getCVFlags() & CV_CONST); }

  // some common queries
  bool isSimpleType() const;
  SimpleType const *asSimpleTypeC() const;
  bool isSimple(SimpleTypeId id) const;
  bool isSimpleCharType() const {return isSimple(ST_CHAR);}
  bool isSimpleWChar_TType() const {return isSimple(ST_WCHAR_T);}
  bool isIntegerType() const;            // any of the simple integer types
  bool isEnumType() const;
  bool isUnionType() const { return isCompoundTypeOf(CompoundType::K_UNION); }
  bool isStructType() const { return isCompoundTypeOf(CompoundType::K_STRUCT); }
  bool isCompoundTypeOf(CompoundType::Keyword keyword) const;
  bool isVoid() const { return isSimple(ST_VOID); }
  bool isBool() const { return isSimple(ST_BOOL); }
  bool isEllipsis() const { return isSimple(ST_ELLIPSIS); }
  bool isError() const { return isSimple(ST_ERROR); }
  bool isDependent() const { return isSimple(ST_DEPENDENT); }
  CompoundType *ifCompoundType();        // NULL or corresp. compound
  CompoundType const *asCompoundTypeC() const; // fail assertion if not
  CompoundType *asCompoundType() { return const_cast<CompoundType*>(asCompoundTypeC()); }
  bool isOwnerPtr() const;

  // pointer/reference stuff
  bool isPointer() const;                // as opposed to reference or non-pointer
  bool isReference() const;
  bool isLval() const { return isReference(); }    // C terminology

  // note that Type overrides these to return Type instead of BaseType
  BaseType const *asRvalC() const;           // if I am a reference, return referrent type
  BaseType *asRval() { return const_cast<BaseType*>(asRvalC()); }

  bool isCVAtomicType(AtomicType::Tag tag) const;
  bool isTypeVariable() const { return isCVAtomicType(AtomicType::T_TYPEVAR); }
  TypeVariable *asTypeVariable();
  bool isCompoundType() const { return isCVAtomicType(AtomicType::T_COMPOUND); }

  bool isTemplateFunction() const;
  bool isTemplateClass() const;

  // this is true if any of the type *constructors* on this type
  // refer to ST_ERROR; we don't dig down inside e.g. members of
  // referred-to classes
  bool containsErrors() const;

  // similar for TypeVariable
  bool containsTypeVariables() const;

  ALLOC_STATS_DECLARE
};

ENUM_BITWISE_OPS(BaseType::EqFlags, BaseType::EF_ALL)


#ifdef TYPE_CLASS_FILE
  // pull in the definition of Type, which may have additional
  // fields (etc.) added by the client
  #include TYPE_CLASS_FILE
#else
  // please see cc_type.html, section 6, "BaseType and Type", for more
  // information about this class
  class Type : public BaseType {
  public:      // types
    typedef bool (*TypePred)(Type const *t);

  protected:   // funcs
    Type() {}

  public:      // funcs
    // filter on all type constructors (i.e. non-atomic types); return
    // true if any constructor satisfies 'pred'
    virtual bool anyCtorSatisfies(TypePred pred) const=0;

    // do not leak the name "BaseType"
    Type const *asRvalC() const
      { return static_cast<Type const *>(BaseType::asRvalC()); }
    Type *asRval()
      { return static_cast<Type*>(BaseType::asRval()); }
  };
#endif // TYPE_CLASS_FILE


// essentially just a wrapper around an atomic type, but
// also with optional const/volatile flags
class CVAtomicType : public Type {
public:     // data
  AtomicType *atomic;          // (serf) underlying type
  CVFlags cv;                  // const/volatile

protected:
  friend class BasicTypeFactory;
  CVAtomicType(AtomicType *a, CVFlags c)
    : atomic(a), cv(c) {}

  // need this to make an array of them
  CVAtomicType(CVAtomicType const &obj)
    : atomic(obj.atomic), cv(obj.cv) {}

public:
  bool innerEquals(CVAtomicType const *obj, EqFlags flags) const;
  bool isConst() const { return !!(cv & CV_CONST); }
  bool isVolatile() const { return !!(cv & CV_VOLATILE); }

  // Type interface
  virtual Tag getTag() const { return T_ATOMIC; }
  virtual string leftString(bool innerParen=true) const;
  virtual int reprSize() const;
  virtual bool anyCtorSatisfies(TypePred pred) const;
  virtual CVFlags getCVFlags() const;
};


// "*" vs "&"
enum PtrOper {
  PO_POINTER, PO_REFERENCE
};

// type of a pointer or reference
class PointerType : public Type {
public:     // data
  PtrOper op;                  // "*" or "&"
  CVFlags cv;                  // const/volatile, if "*"; refers to pointer *itself*
  Type *atType;                // (serf) type of thing pointed-at

protected:  // funcs
  friend class BasicTypeFactory;
  PointerType(PtrOper o, CVFlags c, Type *a);

public:
  bool innerEquals(PointerType const *obj, EqFlags flags) const;
  bool isConst() const { return !!(cv & CV_CONST); }
  bool isVolatile() const { return !!(cv & CV_VOLATILE); }

  // Type interface
  virtual Tag getTag() const { return T_POINTER; }
  virtual string leftString(bool innerParen=true) const;
  virtual string rightString(bool innerParen=true) const;
  virtual int reprSize() const;
  virtual bool anyCtorSatisfies(TypePred pred) const;
  virtual CVFlags getCVFlags() const;
};


// some flags that can be associated with function types
enum FunctionFlags {
  FF_NONE        = 0x00,       // nothing special
  FF_MEMBER      = 0x01,       // function is a nonstatic member
  FF_VARARGS     = 0x02,       // accepts variable # of arguments
  FF_CONVERSION  = 0x04,       // conversion operator function
  FF_CTOR        = 0x08,       // constructor
  FF_DTOR        = 0x10,       // destructor
  FF_ALL         = 0x1F,       // all flags set to 1
};
ENUM_BITWISE_OPS(FunctionFlags, FF_ALL);


// type of a function
class FunctionType : public Type {
public:     // types
  // list of exception types that can be thrown
  class ExnSpec {
  public:
    SObjList<Type> types;

  public:
    ExnSpec() {}
    ExnSpec(ExnSpec const &obj);
    ~ExnSpec();

    bool anyCtorSatisfies(Type::TypePred pred) const;
  };

public:     // data
  // various boolean properties
  FunctionFlags flags;

  // type of return value
  Type *retType;                     // (serf)

  // list of function parameters; if (flags & FF_MEMBER) then the
  // first parameter is 'this'
  SObjList<Variable> params;

  // allowable exceptions, if not NULL
  ExnSpec *exnSpec;                  // (nullable owner)

  // for template functions, this is the list of template parameters
  TemplateParams *templateParams;    // (owner)

protected:  // funcs
  friend class BasicTypeFactory;

  FunctionType(Type *retType);

public:
  virtual ~FunctionType();

  // interpretations of flags
  bool isMember() const               { return !!(flags & FF_MEMBER); }
  bool acceptsVarargs() const         { return !!(flags & FF_VARARGS); }
  bool isConversionOperator() const   { return !!(flags & FF_CONVERSION); }
  bool isConstructor() const          { return !!(flags & FF_CTOR); }
  bool isDestructor() const           { return !!(flags & FF_DTOR); }

  bool innerEquals(FunctionType const *obj, EqFlags flags = EF_EXACT) const;
  bool equalParameterLists(FunctionType const *obj, EqFlags flags = EF_EXACT) const;
  bool equalExceptionSpecs(FunctionType const *obj) const;

  // if the 'this' parameter (if any) is ignored in both function
  // types, am I equal to 'obj'?
  bool equalOmittingThisParam(FunctionType const *obj) const
    { return innerEquals(obj, EF_SKIPTHIS); }

  // append a parameter to the (ordinary) parameters list
  void addParam(Variable *param);

  // add the implicit 'this' param; sets 'isMember()' to true
  void addThisParam(Variable *param);

  // called when we're done adding parameters to this function
  // type, thus any Type annotation system can assume the
  // function type is now completely described
  virtual void doneParams();

  bool isTemplate() const { return templateParams!=NULL; }
  
  CVFlags getThisCV() const;         // dig down; or CV_NONE if !isMember

  Variable const *getThisC() const;  // 'isMember' must be true
  Variable *getThis() { return const_cast<Variable*>(getThisC()); }

  // more specialized printing, for Cqual++ syntax
  virtual string rightStringUpToQualifiers(bool innerParen) const;
  virtual string rightStringAfterQualifiers() const;

  // a hook for the verifier's printer
  virtual void extraRightmostSyntax(stringBuilder &sb) const;

  // Type interface
  virtual Tag getTag() const { return T_FUNCTION; }
  virtual string leftString(bool innerParen=true) const;
  virtual string rightString(bool innerParen=true) const;
  virtual int reprSize() const;
  virtual bool anyCtorSatisfies(TypePred pred) const;
};


// type of an array
class ArrayType : public Type {
public:       // types
  enum { NO_SIZE = -1 };

public:       // data
  Type *eltType;               // (serf) type of the elements
  int size;                    // specified size, or NO_SIZE

private:      // funcs
  void checkWellFormedness() const;

protected:
  friend class BasicTypeFactory;
  ArrayType(Type *e, int s = NO_SIZE)
    : eltType(e), size(s) { checkWellFormedness(); }

public:
  bool innerEquals(ArrayType const *obj, EqFlags flags) const;

  bool hasSize() const { return size != NO_SIZE; }

  // Type interface
  virtual Tag getTag() const { return T_ARRAY; }
  virtual string leftString(bool innerParen=true) const;
  virtual string rightString(bool innerParen=true) const;
  virtual int reprSize() const;
  virtual bool anyCtorSatisfies(TypePred pred) const;
};


// pointer to member; at least for now, this is not unified with
// PointerType because that would make it too easy to have silent bugs
// in code that deals with ptr-to-member, where e.g. something just
// plain fails to check if the pointer is a ptr-to-member
class PointerToMemberType : public Type {
public:
  CompoundType *inClass;        // class whose member is being pointed at
  CVFlags cv;                   // whether this pointer is const
  Type *atType;                 // type of the member

protected:
  friend class BasicTypeFactory;
  PointerToMemberType(CompoundType *c, CVFlags c, Type *a);

public:
  bool innerEquals(PointerToMemberType const *obj, EqFlags flags) const;
  bool isConst() const { return !!(cv & CV_CONST); }

  // Type interface
  virtual Tag getTag() const { return T_POINTERTOMEMBER; }
  virtual string leftString(bool innerParen=true) const;
  virtual string rightString(bool innerParen=true) const;
  virtual int reprSize() const;
  virtual bool anyCtorSatisfies(TypePred pred) const;
  virtual CVFlags getCVFlags() const;
};


// -------------------- templates -------------------------
// used for template parameter types
class TypeVariable : public NamedAtomicType {
public:
  TypeVariable(StringRef name) : NamedAtomicType(name) {}
  ~TypeVariable();

  // AtomicType interface
  virtual Tag getTag() const { return T_TYPEVAR; }
  virtual string toCString() const;
  virtual int reprSize() const;
};


class TemplateParams {
public:
  SObjList<Variable> params;

public:
  TemplateParams() {}
  TemplateParams(TemplateParams const &obj);
  ~TemplateParams();

  string toString() const;
  bool equalTypes(TemplateParams const *obj) const;
  bool anyCtorSatisfies(Type::TypePred pred) const;
};


// semantic template argument (semantic as opposed to syntactic); this
// breaks the argument down into the cases described in cppstd 14.3.2
// para 1, plus types, minus template parameters, then grouped into
// equivalence classes as implied by cppstd 14.4 para 1
class STemplateArgument {
public:
  enum Kind {
    STA_NONE,        // not yet resolved into a valid template argument
    STA_TYPE,        // type argument
    STA_INT,         // int or enum argument
    STA_REFERENCE,   // reference to global object
    STA_POINTER,     // pointer to global object
    STA_MEMBER,      // pointer to class member
    STA_TEMPLATE,    // template argument (not implemented)
    NUM_STA_KINDS
  } kind;

  union {
    Type *t;         // (serf) for STA_TYPE
    int i;           // for STA_INT
    Variable *v;     // (serf) for STA_REFERENCE, STA_POINTER, STA_MEMBER
  } value;

public:
  STemplateArgument() : kind(STA_NONE) { value.i = 0; }
  STemplateArgument(STemplateArgument const &obj);

  // set 'value', ensuring correspondence between it and 'kind'
  void setType(Type *t)          { kind=STA_TYPE;      value.t=t; }
  void setInt(int i)             { kind=STA_INT;       value.i=i; }
  void setReference(Variable *v) { kind=STA_REFERENCE; value.v=v; }
  void setPointer(Variable *v)   { kind=STA_POINTER;   value.v=v; }
  void setMember(Variable *v)    { kind=STA_MEMBER;    value.v=v; }

  bool hasValue() const { return kind!=STA_NONE; }

  // the point of boiling down the syntactic arguments into this
  // simpler semantic form is to make equality checking easy
  bool equals(STemplateArgument const *obj) const;
  
  // debug print
  string toString() const;
};

string sargsToString(SObjList<STemplateArgument> const &list);
inline string sargsToString(ObjList<STemplateArgument> const &list)
  { return sargsToString((SObjList<STemplateArgument> const &)list); }


// for a template class, including instantiations thereof, this is the
// template-related info
class ClassTemplateInfo : public TemplateParams {
public:    // data
  // name of the least-specialized template; this is used to detect
  // when a name is the name of the template we're inside
  StringRef baseName;

  // list of instantiations and specializations of this template class
  SObjList<CompoundType> instantiations;

  // if this is an instantiation or specialization, then this is the
  // list of template arguments
  ObjList<STemplateArgument> arguments;
                                            
  // keep the syntactic arguments around so we can deal with
  // forward-declared templates
  FakeList<TemplateArgument> *argumentSyntax;

public:    // funcs
  ClassTemplateInfo(StringRef baseName);
  ~ClassTemplateInfo();
  
  bool equalArguments(SObjList<STemplateArgument> const &list) const;
};


// ------------------- type factory -------------------
// The type factory is used for constructing objects that represent
// C++ types.  The reasons for using a factory instead of direct
// construction include:
//   - Type have a complicated and unpredictable sharing structure,
//     which makes recursive deallocation impossible.  The factory
//     is thus given responsibility for deallocation of all objects
//     created by that factory.
//   - Types are intended to be immutable, and thus referentially
//     transparent.  This enables the optimization of "hash consing"
//     where multiple requests for the same equivalent object yield
//     the exact same object.  The factory is responsible for
//     maintaining the data structures necessary for this, and for
//     choosing whether to do so at all.
//   - It is often desirable to annotate Types, but the base Type
//     hierarchy should be free from any particular annotations.
//     The factory allows one to derive subclasses of Type to add
//     such annotations, without modifying creation sites.

// first, we have the abstract interface of a TypeFactory
class TypeFactory {
public:
  virtual ~TypeFactory() {}      // silence stupid compiler warnings

  // ---- constructors for the atomic types ----
  // for now, only CompoundType is built this way, and I'm going to
  // provide a default implementation in TypeFactory to avoid having
  // to change the interface w.r.t. cc_qual
  virtual CompoundType *makeCompoundType
    (CompoundType::Keyword keyword, StringRef name);

  // ---- constructors for the constructed types ----
  // the 'loc' being passed is the start of the syntactic construct
  // which causes the type to be created or needed (until I get more
  // experience with this I can't be more precise)
  virtual CVAtomicType *makeCVAtomicType(SourceLoc loc,
    AtomicType *atomic, CVFlags cv)=0;

  virtual PointerType *makePointerType(SourceLoc loc,
    PtrOper op, CVFlags cv, Type *atType)=0;

  virtual FunctionType *makeFunctionType(SourceLoc loc,
    Type *retType)=0;

  virtual ArrayType *makeArrayType(SourceLoc loc,
    Type *eltType, int size = ArrayType::NO_SIZE)=0;

  virtual PointerToMemberType *makePointerToMemberType(SourceLoc loc,
    CompoundType *inClass, CVFlags cv, Type *atType)=0;

  // ---- clone types ----
  // when types are cloned, their location is expected to be copied too
  virtual CVAtomicType *cloneCVAtomicType(CVAtomicType *src)=0;
  virtual PointerType *clonePointerType(PointerType *src)=0;
  virtual FunctionType *cloneFunctionType(FunctionType *src)=0;
  virtual ArrayType *cloneArrayType(ArrayType *src)=0;
  virtual PointerToMemberType *clonePointerToMemberType(PointerToMemberType *src)=0;
  Type *cloneType(Type *src);       // switch, clone, return

  // NOTE: all 'syntax' pointers are nullable, since there are contexts
  // where I don't have an AST node to pass

  // ---- create a type based on another one ----
  // given a type, qualify it with 'cv'; return NULL if the base type
  // cannot be so qualified; I pass the syntax from which the 'cv'
  // flags were derived, when I have it, for the benefit of extension
  // analyses
  virtual Type *applyCVToType(SourceLoc loc, CVFlags cv, Type *baseType,
                              TypeSpecifier * /*nullable*/ syntax);

  // this is called in a few specific circumstances when we want to
  // know the reference type corresponding to some variable; the
  // analysis may need to do something other than a straightforward
  // "makePointerType(PO_REFERENCE, CV_CONST, underlying)" (which is
  // the default behavior); this also has the semantics that if
  // 'underlying' is ST_ERROR then this must return ST_ERROR
  virtual Type *makeRefType(SourceLoc loc, Type *underlying);

  // build a pointer type from a syntactic description; here I allow
  // the factory to know the name of an AST node, but the default
  // implementation will not use it, so it need not be linked in for
  // this to make sense
  virtual PointerType *syntaxPointerType(SourceLoc loc,
    PtrOper op, CVFlags cv, Type *underlying, 
    D_pointer * /*nullable*/ syntax);

  // similar for a function type; the parameters will be added by
  // the caller after this function returns
  virtual FunctionType *syntaxFunctionType(SourceLoc loc,
    Type *retType, D_func * /*nullable*/ syntax, TranslationUnit *tunit);

  // and another for pointer-to-member
  virtual PointerToMemberType *syntaxPointerToMemberType(SourceLoc loc,
    CompoundType *inClass, CVFlags cv, Type *atType, 
    D_ptrToMember * /*nullable*/ syntax);

  // given a class, build the type of the 'this' parameter
  // 1/19/03: it should be a *reference* type
  virtual PointerType *makeTypeOf_this(SourceLoc loc,
    CompoundType *inClass, CVFlags cv, D_func * /*nullable*/ syntax);

  // given a function type and a return type, make a new function type
  // which is like it but has no parameters; i.e., copy all fields
  // except 'params'
  virtual FunctionType *makeSimilarFunctionType(SourceLoc loc,
    Type *retType, FunctionType *similar);

  // ---- similar functions for Variable ----
  // Why not make a separate factory?
  //   - It's inconvenient to have two.
  //   - Every application I can think of will want to define both
  //     or neither.
  //   - Variable is used by Type and vice-versa.. they could have
  //     both been defined in cc_type.h
  virtual Variable *makeVariable(SourceLoc L, StringRef n,
                                 Type *t, DeclFlags f, TranslationUnit *tunit)=0;
  virtual Variable *cloneVariable(Variable *src)=0;


  // ---- convenience functions ----
  // these functions are implemented in TypeFactory directly, in
  // terms of the interface above, so they're not virtual

  // given an AtomicType, wrap it in a CVAtomicType
  // with no const or volatile qualifiers
  CVAtomicType *makeType(SourceLoc loc, AtomicType *atomic)
    { return makeCVAtomicType(loc, atomic, CV_NONE); }

  // make a ptr-to-'type' type; returns generic Type instead of
  // PointerType because sometimes I return fixed(ST_ERROR)
  inline Type *makePtrType(SourceLoc loc, Type *type)
    { return type->isError()? type : makePointerType(loc, PO_POINTER, CV_NONE, type); }

  // map a simple type into its CVAtomicType representative
  CVAtomicType *getSimpleType(SourceLoc loc, SimpleTypeId st, CVFlags cv = CV_NONE);

  // given an array type with no size, return one that is
  // the same except its size is as specified
  ArrayType *setArraySize(SourceLoc loc, ArrayType *type, int size);
};


// this is an implementation of the above interface which
// returns the actual Type-derived objects defined, as opposed
// to objects of further-derived classes
class BasicTypeFactory : public TypeFactory {
private:   // data
  // global array of non-const, non-volatile built-ins; it's expected
  // to be treated as read-only, but enforcement (naive 'const' would
  // not work because Type* aren't const above)
  static CVAtomicType unqualifiedSimple[NUM_SIMPLE_TYPES];

public:    // funcs
  // TypeFactory funcs
  virtual CVAtomicType *makeCVAtomicType(SourceLoc loc, 
    AtomicType *atomic, CVFlags cv);
  virtual PointerType *makePointerType(SourceLoc loc, 
    PtrOper op, CVFlags cv, Type *atType);
  virtual FunctionType *makeFunctionType(SourceLoc loc,
    Type *retType);
  virtual ArrayType *makeArrayType(SourceLoc loc,
    Type *eltType, int size);
  virtual PointerToMemberType *makePointerToMemberType(SourceLoc loc,
    CompoundType *inClass, CVFlags cv, Type *atType);

  virtual CVAtomicType *cloneCVAtomicType(CVAtomicType *src);
  virtual PointerType *clonePointerType(PointerType *src);
  virtual FunctionType *cloneFunctionType(FunctionType *src);
  virtual ArrayType *cloneArrayType(ArrayType *src);
  virtual PointerToMemberType *clonePointerToMemberType(PointerToMemberType *src);

  virtual Variable *makeVariable(SourceLoc L, StringRef n,
                                 Type *t, DeclFlags f, TranslationUnit *tunit);
  virtual Variable *cloneVariable(Variable *src);
};


// ------ for debugging ------
// The idea here is you say "print type_toString(x)" in gdb, where 'x'
// is a pointer to some type.  The pointer that is returned will be
// printed as a string by gdb.
//
// update: it turns out the latest gdbs are capable of using the
// toString method directly!  but I'll leave this here anyway.
char *type_toString(Type const *t);


#endif // CC_TYPE_H
