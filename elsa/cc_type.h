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
#include "srcloc.h"       // SourceLoc
#include "exc.h"          // xBase
#include "serialno.h"     // INHERIT_SERIAL_BASE

class Variable;           // variable.h
class MatchBindings;      // matchtype.h
class Env;                // cc_env.h
class TS_classSpec;       // cc.ast
class Expression;         // cc.ast
class TemplateArgument;   // cc.ast
class D_pointer;          // cc.ast
class D_func;             // cc.ast
class D_ptrToMember;      // cc.ast
class TypeSpecifier;      // cc.ast
class Declaration;        // cc.ast

// fwd in this file
class SimpleType;
class NamedAtomicType;
class CompoundType;
class BaseClass;
class EnumType;
class TypeVariable;
class CVAtomicType;
class PointerType;
class ReferenceType;
class FunctionType;
class ArrayType;
class PointerToMemberType;
class Type;
class TemplateInfo;
class STemplateArgument;
class TypeFactory;
class BasicTypeFactory;
class TypePred;


// FIX: eventually perhaps we can remove this
extern TypeFactory * global_tfac;


// --------------------- atomic types --------------------------
// interface to types that are atomic in the sense that no
// modifiers can be stripped away; see types.txt
class AtomicType {
public:     // types
  enum Tag { T_SIMPLE, T_COMPOUND, T_ENUM, T_TYPEVAR, NUM_TAGS };

public:     // funcs
  AtomicType();
  virtual ~AtomicType();

  virtual Tag getTag() const = 0;
  bool isSimpleType() const   { return getTag() == T_SIMPLE; }
  bool isCompoundType() const { return getTag() == T_COMPOUND; }
  bool isEnumType() const     { return getTag() == T_ENUM; }
  bool isTypeVariable() const { return getTag() == T_TYPEVAR; }
  virtual bool isNamedAtomicType() const;    // default impl returns false

  // see smbase/macros.h
  DOWNCAST_FN(SimpleType)
  DOWNCAST_FN(NamedAtomicType)
  DOWNCAST_FN(CompoundType)
  DOWNCAST_FN(EnumType)
  DOWNCAST_FN(TypeVariable)

  // this is type equality, *not* coercibility -- e.g. if
  // we say "extern type1 x" and then "extern type2 x" we
  // will allow it only if type1==type2
  bool equals(AtomicType const *obj) const;

  // print the string according to 'Type::printAsML'
  string toString() const;

  // print in C notation
  virtual string toCString() const = 0;

  // print in ML notation
  virtual string toMLString() const = 0;
  
  // toString()+newline to cout
  void gdb() const;

  // size this type's representation occupies in memory; this
  // might throw XReprSize, see below
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
  virtual string toMLString() const;
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

  virtual bool isNamedAtomicType() const;       // returns true
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

  // list of all conversion operators this class has, including
  // those that have been inherited but not then hidden
  SObjList<Variable> conversionOperators;

  // if this class is a template instantiation, 'instName' is
  // the class name plus a rendering of the arguments; otherwise
  // it's the same as 'name'; this is for debugging
  StringRef instName;

  // AST node that describes this class; used for implementing
  // templates (AST pointer)
  // dsw: used for other purposes also
  TS_classSpec *syntax;               // (serf)

private:     // funcs
  void makeSubobjHierarchy(BaseClassSubobj *subobj, BaseClass const *newBase);

  BaseClassSubobj const *findVirtualSubobjectC(CompoundType const *ct) const;
  BaseClassSubobj *findVirtualSubobject(CompoundType *ct)
    { return const_cast<BaseClassSubobj*>(findVirtualSubobjectC(ct)); }

  static void clearVisited_helper(BaseClassSubobj const *subobj);

  static void getSubobjects_helper
    (SObjList<BaseClassSubobj const> &dest, BaseClassSubobj const *subobj);

  void addLocalConversionOp(Variable *op);

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

  // manipulate 'templateInfo' which is now in Variable
  TemplateInfo *templateInfo() const;
  void setTemplateInfo(TemplateInfo *templInfo0);

  // same as 'typedefVar' but with a bunch of assertions...
  Variable *getTypedefVar() const;

  // true if the class has RTTI/vtable
  bool hasVirtualFns() const;

  static char const *keywordName(Keyword k);

  // AtomicType interface
  virtual Tag getTag() const { return T_COMPOUND; }
  virtual string toCString() const;
  virtual string toMLString() const;
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
  SObjList<Variable> &getDataVariablesInOrder() 
    { return dataMembers; }

  // return the ordinal position of the named nonstatic data field
  // in this class, starting with 0; or, return -1 if the field does
  // not exist (this is a query on 'dataMembers')
  int getDataMemberPosition(StringRef name) const;

  // add to 'bases'; incrementally maintains 'virtualBases'
  virtual void addBaseClass(BaseClass * /*owner*/ newBase);

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
  bool hasStrictBaseClass(CompoundType const *ct) const
    { return this != ct && hasBaseClass(ct); }

  // compute the least upper bound of two compounds in the inheritance
  // network; 'wasAmbig==true' means that the intersection of the
  // superclasses was not empty, but that set had no least element;
  // return NULL if no LUB ("least" means most-derived)
  static CompoundType *lub(CompoundType *t1, CompoundType *t2, bool &wasAmbig);

  // some special member functions; these might be NULL at early
  // stages of processing, but the type checker fills them in at some
  // point (addCompilerSuppliedDecls)
  Variable *getDefaultCtor();         // C(); might be NULL at any time
  Variable *getCopyCtor();            // C(C const &);
  Variable *getAssignOperator();      // C& operator= (C const &);
  Variable *getDtor();                // ~C();

  // call this when we're finished adding base classes and member
  // fields; it builds 'conversionOperators'; 'specialName' is the
  // name under which the conversion operators have been filed in
  // the class scope
  virtual void finishedClassDefinition(StringRef specialName);
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
  virtual string toMLString() const;
  virtual int reprSize() const;

  Value *addValue(StringRef name, int value, /*nullable*/ Variable *d);
  Value const *getValue(StringRef name) const;
};


// ------------------- constructed types -------------------------
// generic constructed type; to allow client analyses to annotate the
// description of types, this class is inherited by "Type", the class
// that all of the rest of the parser regards as being a "type"
//
// note: clients should refer to Type, not BaseType
class BaseType INHERIT_SERIAL_BASE {
public:     // types
  enum Tag { 
    T_ATOMIC, 
    T_POINTER,
    T_REFERENCE,
    T_FUNCTION,
    T_ARRAY,
    T_POINTERTOMEMBER
  };

public:     // data
  // This is a list of typedef'd aliases for this type.  It may be
  // empty.  For compound types, there will be at least one, and it
  // (the first one) will be the same as the corresponding
  // NamedAtomicType::typedefVar (this is not so much an invariant to
  // be relied upon as an explanation of the difference between them).
  //
  // The reason this was introduced is to make it easier to build an
  // AST node referring to a given type (i.e. can just use TS_name
  // instead of building the whole thing) during elaboration.  It also
  // can be used to make error messages that use the programmer's
  // names instead of their normal form.
  //
  // TODO: Now that elaboration doesn't need this, remove it.
  SObjList<Variable> typedefAliases;

  // moved this declaration into Type, along with the declaration of
  // 'anyCtorSatisfies', so as not to leak the name "BaseType"
  //typedef bool (*TypePred)(Type const *t);

  // when true (the default is false), types are printed in ML-like
  // notation instead of C notation by AtomicType::toString and
  // Type::toString
  static bool printAsML;

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
  bool isReferenceType() const { return getTag() == T_REFERENCE; }
  bool isFunctionType() const { return getTag() == T_FUNCTION; }
  bool isArrayType() const { return getTag() == T_ARRAY; }
  bool isPointerToMemberType() const { return getTag() == T_POINTERTOMEMBER; }

  DOWNCAST_FN(CVAtomicType)
  DOWNCAST_FN(PointerType)
  DOWNCAST_FN(ReferenceType)
  DOWNCAST_FN(FunctionType)
  DOWNCAST_FN(ArrayType)
  DOWNCAST_FN(PointerToMemberType)

  // flags to control how "equal" the types must be; also see
  // note above FunctionType::innerEquals
  enum EqFlags {                   
    // complete equality; this is the default; note that we are
    // checking for *type* equality, rather than equality of the
    // syntax used to denote it, so we do *not* compare:
    //   - function parameter names
    //   - typedef usage
    EF_EXACT           = 0x0000,

    // ----- basic behaviors -----
    // when comparing function types, do not check whether the
    // return types are equal
    EF_IGNORE_RETURN   = 0x0001,

    // when comparing function types, if one is a nonstatic member
    // function and the other is not, then do not (necessarily)
    // call them unequal
    EF_STAT_EQ_NONSTAT = 0x0002,    // static can equal nonstatic

    // when comparing function types, only compare the explicit
    // (non-receiver) parameters; this does *not* imply
    // EF_STAT_EQ_NONSTAT
    EF_IGNORE_IMPLICIT = 0x0004,

    // ignore the topmost cv qualifications of all parameters in
    // parameter lists throughout the type
    EF_IGNORE_PARAM_CV = 0x0008,

    // ignore the topmost cv qualification of the two types compared
    EF_IGNORE_TOP_CV   = 0x0010,

    // when comparing function types, ignore the exception specs
    EF_IGNORE_EXN_SPEC = 0x0020,

    // allow the cv qualifications to differ up to the first type
    // constructor that is not a pointer or pointer-to-member; this
    // is cppstd 4.4 para 4 "similar"; implies EF_IGNORE_TOP_CV
    EF_SIMILAR         = 0x0040,

    // when the second type in the comparison is polymorphic (for
    // built-in operators; this is not for templates), and the first
    // type is in the set of types described, say they're equal;
    // note that polymorhism-enabled comparisons therefore are not
    // symmetric in their arguments
    EF_POLYMORPHIC     = 0x0080,

    // In K&R mode to allow for the fact that, in some circumstances,
    // a function can be declared more than once but only some times
    // provide a parameter list and other times provide an empty
    // argument list, which in K&R mode this typechecks with a
    // parameter list of '(...)'.  Thus in K&R mode, we allow two such
    // function types to be considered equal.
    EF_ALLOW_KR_PARAM_OMIT = 0x0100,

    // ----- combined behaviors -----
    // all flags set to 1
    EF_ALL             = 0x01FF,

    // signature equivalence for the purpose of detecting whether
    // two declarations refer to the same entity (as opposed to two
    // overloaded entities)
    EF_SIGNATURE       = (
      EF_IGNORE_RETURN |       // can't overload on return type
      EF_IGNORE_PARAM_CV |     // param cv doesn't matter
      EF_STAT_EQ_NONSTAT |     // can't overload on static vs. nonstatic
      EF_IGNORE_EXN_SPEC       // can't overload on exn spec
    ),

    // ----- combinations used by the equality implementation -----
    // this is the set of flags that allow CV variance within the
    // current type constructor
    EF_OK_DIFFERENT_CV = (EF_IGNORE_TOP_CV | EF_SIMILAR),

    // this is the set of flags that automatically propagate down
    // the type tree equality checker; others are suppressed once
    // the first type constructor looks at them
    EF_PROP            = (EF_IGNORE_PARAM_CV | EF_POLYMORPHIC | EF_ALLOW_KR_PARAM_OMIT),
    
    // these flags are propagated below ptr and ptr-to-member
    EF_PTR_PROP        = (EF_PROP | EF_SIMILAR | EF_ALLOW_KR_PARAM_OMIT)
  };

  // like above, this is (structural) equality, not coercibility;
  // internally, this calls the innerEquals() method on the two
  // objects, once their tags have been established to be equal
  bool equals(BaseType const *obj, EqFlags flags = EF_EXACT) const;

  // compute a hash value: equal types (EF_EXACT) have the same hash
  // value, and unequal types are likely to have different values
  unsigned hashValue() const;
  virtual unsigned innerHashValue() const = 0;

  // print the string according to 'printAsML'
  string toString() const;

  // print the type, with an optional name like it was a declaration
  // for a variable of that type
  string toCString() const;
  string toCString(char const *name) const;

  // NOTE: yes, toMLString() is virtual, whereas toCString() is not
  virtual string toMLString() const = 0;
  void putSerialNo(stringBuilder &sb) const;
  
  // toString+newline to cout
  void gdb() const;

  // the left/right business is to allow us to print function
  // and array types in C's syntax; if 'innerParen' is true then
  // the topmost type constructor should print the inner set of
  // paretheses
  virtual string leftString(bool innerParen=true) const = 0;
  virtual string rightString(bool innerParen=true) const;    // default: returns ""

  // size of representation at run-time; for now uses nominal 32-bit
  // values
  virtual int reprSize() const = 0;

  // see description below, in class Type
  //virtual bool anyCtorSatisfies(TypePred pred) const=0;

  // return the cv flags that apply to this type, if any;
  // default returns CV_NONE
  virtual CVFlags getCVFlags() const;
  bool isConst() const { return !!(getCVFlags() & CV_CONST); }

  // some common queries
  bool isSimpleType() const;
  SimpleType const *asSimpleTypeC() const;
  bool isSimple(SimpleTypeId id) const;
  bool isSimpleCharType() const { return isSimple(ST_CHAR); }
  bool isSimpleWChar_tType() const { return isSimple(ST_WCHAR_T); }
  bool isIntegerType() const;            // any of the simple integer types
  bool isEnumType() const;
  bool isUnionType() const { return isCompoundTypeOf(CompoundType::K_UNION); }
  bool isStructType() const { return isCompoundTypeOf(CompoundType::K_STRUCT); }
  bool isCompoundTypeOf(CompoundType::Keyword keyword) const;
  bool isVoid() const { return isSimple(ST_VOID); }
  bool isBool() const { return isSimple(ST_BOOL); }
  bool isEllipsis() const { return isSimple(ST_ELLIPSIS); }
  bool isError() const { return isSimple(ST_ERROR); }
  bool isDependent() const;
  bool isOwnerPtr() const;
  bool isMethod() const;                       // function and method

  // (some of the following are redundant but I want them anyway, to
  // continue with the pattern that isXXX is for language concepts and
  // isXXXType is for implementation concepts)

  // pointer/reference stuff
  bool isPointer() const { return isPointerType(); }
  bool isReference() const { return isReferenceType(); }
  bool isReferenceToConst() const;
  bool isLval() const { return isReference(); }// C terminology

  // allow some degree of unified handling of PointerType and ReferenceType
  bool isPtrOrRef() const { return isPointer() || isReference(); }
  Type *getAtType() const;

  // note that Type overrides these to return Type instead of BaseType
  BaseType const *asRvalC() const;             // if I am a reference, return referrent type
  BaseType *asRval() { return const_cast<BaseType*>(asRvalC()); }

  bool isCVAtomicType(AtomicType::Tag tag) const;
  bool isTypeVariable() const { return isCVAtomicType(AtomicType::T_TYPEVAR); }
  TypeVariable *asTypeVariable();

  // downcast etc. to NamedAtomicType
  bool isNamedAtomicType() const;
  NamedAtomicType *ifNamedAtomicType();  // NULL or corresp. NamedAtomicType
  NamedAtomicType const *asNamedAtomicTypeC() const;
  NamedAtomicType *asNamedAtomicType() { return const_cast<NamedAtomicType*>(asNamedAtomicTypeC()); }

  // similar for CompoundType
  bool isCompoundType() const { return isCVAtomicType(AtomicType::T_COMPOUND); }
  CompoundType *ifCompoundType();        // NULL or corresp. compound
  CompoundType const *asCompoundTypeC() const; // fail assertion if not
  CompoundType *asCompoundType() { return const_cast<CompoundType*>(asCompoundTypeC()); }

  // this is true if any of the type *constructors* on this type
  // refer to ST_ERROR; we don't dig down inside e.g. members of
  // referred-to classes
  bool containsErrors() const;

  // similar for TypeVariable
  bool containsTypeVariables() const;

  // returns true if contains type variables or STemplateArgument-s
  // with the tag STA_REFERENCE, since these are what template object
  // (or at least int) variables look like.  Recurses down into
  // TemplateArguments.
  bool containsVariables() const;

  // get the first typedef alias, if any
  Variable *typedefName();

  ALLOC_STATS_DECLARE
};

ENUM_BITWISE_OPS(BaseType::EqFlags, BaseType::EF_ALL)


#ifdef TYPE_CLASS_FILE
  // pull in the definition of Type, which may have additional
  // fields (etc.) added for the client analysis
  #include TYPE_CLASS_FILE
#else
  // please see cc_type.html, section 6, "BaseType and Type", for more
  // information about this class
  class Type : public BaseType {
  protected:   // funcs
    Type() {}

  public:      // funcs
    // filter on all constructed types that appear in the type,
    // *including* parameter types; return true if any constructor
    // satisfies 'pred' (note that recursive types always go through
    // CompoundType, and this does not dig into the fields of
    // CompoundTypes)
    virtual bool anyCtorSatisfies(TypePred &pred) const=0;

    // do not leak the name "BaseType"
    Type const *asRvalC() const
      { return static_cast<Type const *>(BaseType::asRvalC()); }
    Type *asRval()
      { return static_cast<Type*>(BaseType::asRval()); }
  };
#endif // TYPE_CLASS_FILE

// ------------------ Type predicates --------------

// It caused too much difficulty in the Oink build process to have
// these as members of class Type.

typedef bool (*TypePredFunc)(Type const *t);

// abstract superclass
class TypePred {
  public:
  virtual bool operator() (Type const *t) = 0;
  virtual ~TypePred() {}
};

// when you just want a stateless predicate
class StatelessTypePred : public TypePred {
  TypePredFunc const f;
  public:
  StatelessTypePred(TypePredFunc f0) : f(f0) {}
  virtual bool operator() (Type const *t) { return f(t); }
  virtual ~StatelessTypePred() {}
};


// supports the use of 'Type*' in AST constructor argument lists
string toString(Type *t);


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

  // need this to make a static array of them
  CVAtomicType(CVAtomicType const &obj)
    : atomic(obj.atomic), cv(obj.cv) {}

public:
  bool innerEquals(CVAtomicType const *obj, EqFlags flags) const;
  bool isConst() const { return !!(cv & CV_CONST); }
  bool isVolatile() const { return !!(cv & CV_VOLATILE); }

  // Type interface
  virtual Tag getTag() const { return T_ATOMIC; }
  unsigned innerHashValue() const;
  virtual string toMLString() const;
  virtual string leftString(bool innerParen=true) const;
  virtual int reprSize() const;
  virtual bool anyCtorSatisfies(TypePred &pred) const;
  virtual CVFlags getCVFlags() const;
};

             
#if 0    // obsolete?
// "*" vs "&"
enum PtrOper {
  PO_POINTER, PO_REFERENCE
};
#endif // 0

// type of a pointer
class PointerType : public Type {
public:     // data
  CVFlags cv;                  // const/volatile; refers to pointer *itself*
  Type *atType;                // (serf) type of thing pointed-at

protected:  // funcs
  friend class BasicTypeFactory;
  PointerType(CVFlags c, Type *a);

public:
  bool innerEquals(PointerType const *obj, EqFlags flags) const;
  bool isConst() const { return !!(cv & CV_CONST); }
  bool isVolatile() const { return !!(cv & CV_VOLATILE); }

  // Type interface
  virtual Tag getTag() const { return T_POINTER; }
  unsigned innerHashValue() const;
  virtual string toMLString() const;
  virtual string leftString(bool innerParen=true) const;
  virtual string rightString(bool innerParen=true) const;
  virtual int reprSize() const;
  virtual bool anyCtorSatisfies(TypePred &pred) const;
  virtual CVFlags getCVFlags() const;
};


// type of a reference
class ReferenceType : public Type {
public:     // data
  Type *atType;                // (serf) type of thing pointed-at

protected:  // funcs
  friend class BasicTypeFactory;
  ReferenceType(Type *a);

public:
  bool innerEquals(ReferenceType const *obj, EqFlags flags) const;
  bool isConst() const { return false; }
  bool isVolatile() const { return false; }

  // Type interface
  virtual Tag getTag() const { return T_REFERENCE; }
  unsigned innerHashValue() const;
  virtual string toMLString() const;
  virtual string leftString(bool innerParen=true) const;
  virtual string rightString(bool innerParen=true) const;
  virtual int reprSize() const;
  virtual bool anyCtorSatisfies(TypePred &pred) const;
  virtual CVFlags getCVFlags() const;
};


// some flags that can be associated with function types
enum FunctionFlags {
  FF_NONE        = 0x00,       // nothing special
  FF_METHOD      = 0x01,       // function is a nonstatic method
  FF_VARARGS     = 0x02,       // accepts variable # of arguments
  FF_CONVERSION  = 0x04,       // conversion operator function
  FF_CTOR        = 0x08,       // constructor
  FF_DTOR        = 0x10,       // destructor
  FF_BUILTINOP   = 0x20,       // built-in operator function (cppstd 13.6)
  FF_ALL         = 0x3F,       // all flags set to 1
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

    bool anyCtorSatisfies(TypePred &pred) const;
  };

public:     // data
  // various boolean properties
  FunctionFlags flags;

  // type of return value
  Type *retType;                     // (serf)

  // list of function parameters; if (flags & FF_METHOD) then the
  // first parameter is '__receiver'
  SObjList<Variable> params;

  // allowable exceptions, if not NULL
  ExnSpec *exnSpec;                  // (nullable owner)

protected:  // funcs
  friend class BasicTypeFactory;

  FunctionType(Type *retType);

public:
  virtual ~FunctionType();

  // interpretations of flags
  bool isMethod() const               { return !!(flags & FF_METHOD); }
  bool acceptsVarargs() const         { return !!(flags & FF_VARARGS); }
  bool isConversionOperator() const   { return !!(flags & FF_CONVERSION); }
  bool isConstructor() const          { return !!(flags & FF_CTOR); }
  bool isDestructor() const           { return !!(flags & FF_DTOR); }

  bool innerEquals(FunctionType const *obj, EqFlags flags = EF_EXACT) const;
  bool equalParameterLists(FunctionType const *obj, EqFlags flags = EF_EXACT) const;
  bool equalExceptionSpecs(FunctionType const *obj) const;

  // if the '__receiver' parameter (if any) is ignored in both
  // function types, am I equal to 'obj'?
  bool equalOmittingReceiver(FunctionType const *obj) const
    { return innerEquals(obj, EF_STAT_EQ_NONSTAT | EF_IGNORE_IMPLICIT); }
                                             
  // true if all parameters after 'startParam' (0 is first) have
  // default values      
  bool paramsHaveDefaultsPast(int startParam) const;

  // append a parameter to the (ordinary) parameters list
  void addParam(Variable *param);

  // add the implicit '__receiver' param; sets 'isMember()' to true
  void addReceiver(Variable *param);

  // called when we're done adding parameters to this function
  // type, thus any Type annotation system can assume the
  // function type is now completely described
  virtual void doneParams();

  // During elaboration, functions that return CompoundTypes are
  // changed to behave as if they instead accepted a reference to an
  // object in which the return value is constructed.  At that time,
  // this function is called, and passed the Variable that is to be
  // bound to that passed object (sort of like a hidden parameter).
  // The base tcheck module does not need to store this, so the
  // default implementation does nothing, but other analyses may want
  // to.
  virtual void registerRetVal(Variable *retVal);

  Variable const *getReceiverC() const;  // 'isMember' must be true
  Variable *getReceiver() { return const_cast<Variable*>(getReceiverC()); }

  CVFlags getReceiverCV() const;         // dig down; or CV_NONE if !isMember
  CompoundType *getClassOfMember();      // 'isMember' must be true

  // more specialized printing, for Cqual++ syntax
  virtual string rightStringUpToQualifiers(bool innerParen) const;
  virtual string rightStringAfterQualifiers() const;

  // a hook for the verifier's printer
  virtual void extraRightmostSyntax(stringBuilder &sb) const;

  // Type interface
  virtual Tag getTag() const { return T_FUNCTION; }
  unsigned innerHashValue() const;
  virtual string toMLString() const;
  virtual string leftString(bool innerParen=true) const;
  virtual string rightString(bool innerParen=true) const;
  virtual int reprSize() const;
  virtual bool anyCtorSatisfies(TypePred &pred) const;
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
  unsigned innerHashValue() const;
  virtual string toMLString() const;
  virtual string leftString(bool innerParen=true) const;
  virtual string rightString(bool innerParen=true) const;
  virtual int reprSize() const;
  virtual bool anyCtorSatisfies(TypePred &pred) const;
};


// pointer to member
class PointerToMemberType : public Type {
public:
  // Usually, this is a compound type, as ptr-to-members are
  // w.r.t. some compound.  However, to support the 'compound'
  // being a template parameter, we generalize slightly so that
  // a TypeVariable can be the 'inClass'.
  NamedAtomicType *inClassNAT;

  CVFlags cv;                   // whether this pointer is const
  Type *atType;                 // type of the member

protected:
  friend class BasicTypeFactory;
  PointerToMemberType(NamedAtomicType *inClassNAT0, CVFlags c, Type *a);

public:
  bool innerEquals(PointerToMemberType const *obj, EqFlags flags) const;
  bool isConst() const { return !!(cv & CV_CONST); }

  // use this in contexts where the 'inClass' is known to be
  // a real compound (which is most contexts); this fails an
  // assertion if it turns out to be a TypeVariable
  CompoundType *inClass() const;

  // Type interface
  virtual Tag getTag() const { return T_POINTERTOMEMBER; }
  unsigned innerHashValue() const;
  virtual string toMLString() const;
  virtual string leftString(bool innerParen=true) const;
  virtual string rightString(bool innerParen=true) const;
  virtual int reprSize() const;
  virtual bool anyCtorSatisfies(TypePred &pred) const;
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
  virtual string toMLString() const;
  virtual int reprSize() const;
};


// just some template parameters (this class exists, in part, so
// that Scope doesn't have to instantiate a full TemplateInfo)
class TemplateParams {
public:    // data
  // template parameters: the dimensions along which the associated
  // entity may be specialized at compile time
  SObjList<Variable> params;

public:    // funcs
  TemplateParams() {}
  TemplateParams(TemplateParams const &obj);
  ~TemplateParams();

  // queries on parameters
  string paramsToCString() const;
  string paramsToMLString() const;
  bool anyParamCtorSatisfies(TypePred &pred) const;
};


// for a template function or class, including instantiations thereof,
// this is the information regarding its template-ness
class TemplateInfo : public TemplateParams {
public:    // data
  // name of the least-specialized ("primary") template; this is used
  // to detect when a name is the name of the template we're inside
  // (this is NULL for TemplateInfos attached to FunctionTypes)
  StringRef baseName;

  // whereas for classes the 'baseName' suffices to create a
  // forward-declared version of the object, functions need
  // the entire declaration; so here it is
  Declaration *declSyntax;          // (serf AST)

private:                        // go through the accessors
  // The primary of this template info if we are a specialization or
  // an instantiation; NULL if we are a primary.
  TemplateInfo *myPrimary;

public:
  // The specialization / primary that we were instantiated from, if
  // we are an instantiation; NULL if we are not
  Variable *instantiatedFrom;

  // NOTE: There is an over-orthogonalization here.  The semantics of
  // instantiations and arguments are as follows; all other
  // combinations are illegal.  The astute reader will note that there
  // is no circumstance under which both instantiations and arguments
  // are non-empty.
  //
  // parameters | instantiations     | arguments            : role
  // -----------+--------------------+----------------------:------------------------
  // non-empty  | possibly non-empty | empty                : primary template
  // non-empty  | empty              | containVariables     : partial specialization
  // empty      | empty              | not containVariables : complete specialization or
  //                                                          instantiation
  // empty      | empty              | containVariables     : mutant, see below
  //
  // NOTE: during the typechecking of a primary, a mutant
  // instantiation is created; It has no parameters (like an
  // instantiation or complete specialization) but *does* have a
  // non-empty arguments list containing type variables.  This mutant
  // cannot be immediately destroyed, as some templates refer to
  // themselves; finding a place where it can be has proven
  // prohibitively difficult and so we simply filter them out before
  // template specialization resolution is done.

  // list of instantiations and specializations of this template IF it
  // is a PRIMARY; the specializations or instantiations of a
  // specialization, S, go under the instantiations of S's primary, P,
  // NOT under S's instantiations
  //
  // This is private so that you have to go through my accessor
  // methods.  One thing this does is that it ensures that it is
  // impossible to insert a member into the instantiations list
  // without setting its myPrimary field to the primary
  private:
  SObjList<Variable> instantiations;

  public:

  // if this is an instantiation or specialization, then this is the
  // list of template arguments; note that an argument for a
  // specialization is known as as "specialization pattern" and may
  // contain typevars.
  //
  // Q: do they line up with 'params'?  what is the relationship
  // between 'params' and 'arguments'?
  //
  // A: The relationship between arguments and params seems to be as
  // follows.  A primary has params and no arguments and any
  // instantiation of it must supply values for those params.
  //
  // An instantiation of the primary does that, and therefore the
  // number of arguments to an instantiation is the same as the number
  // of params of the primary.
  //
  // A complete specialization is an alternate implementation for the
  // body of the primary that should be used when an instantiation of
  // the primary is requested with a particular set of arguments.  The
  // syntax indicates this by prefixing with "template <>" which adds
  // no parameters, and suffixing the name with "<(args_here)>" where
  // "args_here" is the list of arguments such that when an
  // instantiation of the primary is requested that *exactly* matches
  // that list, the body of the complete specialization is used
  // instead of that of the primary.  Note that the number of
  // arguments again must match the number of parameters in the
  // primary, and that the number of parameters is zero.  [That
  // paragraph could be more efficient.]
  //
  // A partial specialization is like a complete specialization except
  // that the arguments may contains variables, such as type variables
  // for type arguments [FIX: how does this work for other types of
  // arguments?] and are therefore known as "specialization patterns".
  // Exactly the union of these free variables in the patterns *must*
  // be listed in a "template<(vars_here)>" prefix to the partial
  // specialization and the *may* be used in the body just as in a
  // primary template.  When an instantiation of the primary is
  // requested and the arguments "fit the pattern" when type
  // unification is requested, the partial specialization *may* [more
  // below] be used.  Bindings of the variables to parts of the actual
  // arguments are created when unification with the actual arguments
  // is performed, such that if those bindings were substituted in for
  // the variables in the specialization patterns, the would match the
  // actual arguments exactly.  The partial specialization is then
  // instantiated just as a primary and this is used in place of
  // instantiating the primary.  Therefore, for a partial
  // specialization, the number of arguments again must match the
  // number of parameters of the primary, and the number of parameters
  // is the number of free variables in the arguments (specialization
  // patterns), which can be more or less than the number of
  // parameters to the primary.
  //
  // A note on ambiguity and its resolution: the astute reader will
  // note that partial specializations make this process ambiguous;
  // often there are more than one partial specialization that could
  // apply.  Note that specialization patterns form a partial order
  // under the "is more specific than" relation.  For class templates,
  // there must be a unique "most specific" pattern that matches the
  // arguments or the call is ambiguous and illegal.  For function
  // templates, all the elements in the partial order that can unify
  // with the arguments are thrown into the overload set for that
  // function along with any other non-template functions and normal
  // function overloading resolution is done.  During this process a
  // template instantiation always looses to a normal function, but if
  // it comes down to two template functions, then they are
  // disambiguated according to "is more specific than", as with class
  // templates.
  ObjList<STemplateArgument> arguments;
  
  // if this is an instantiation, this records the (most proximal)
  // source location that gave rise to the need to instantiate it;
  // if not, it's just the location of the declaration of the
  // template itself
  SourceLoc instLoc;

public:    // funcs
  TemplateInfo(StringRef baseName, SourceLoc instLoc);
  TemplateInfo(TemplateInfo const &obj);
  ~TemplateInfo();

  // just sets 'myPrimary', plus a couple assertions regarding 'prim'
  void setMyPrimary(TemplateInfo *prim);
  // return myPrimary unless we are a primary, in which case return
  // 'this' (since myPrimary for a primary is NULL)
  TemplateInfo *getMyPrimaryIdem() const;

  // add to the instantiation list; supress duplicates by assigning to
  // the reference 'inst0' only if suppressDup is true
  Variable *addInstantiation(Variable *inst0, bool suppressDup=false);
  // only use this for iteration, not appending!  Don't know a good
  // way to enforce that
  SObjList<Variable> &getInstantiations();
  // get the instantiation that matches 'var' in type (types are
  // MM_ISO) and in template arguments; handy for finding the previous
  // declaration of a function to hook up the definition to
  Variable *getInstantiationOfVar(Variable *var);

  // true if 'list' contains equivalent semantic arguments
  bool equalArguments(SObjList<STemplateArgument> const &list) const;

  // check the arguments contain type variables
  bool argumentsContainTypeVariables() const;

  // dsw: check the arguments contain type or object (say, int)
  // variables; FIX: I'm not sure this is implemented right; see
  // comments in implementation
  bool argumentsContainVariables() const;

  bool isMutant() const;
  bool isPrimary() const;
  bool isNotPrimary() const;    // "!isPrimary()" fails on mutants
  bool isPartialSpec() const;
  bool isCompleteSpecOrInstantiation() const;

  // debugging
  void gdb();
  void debugPrint(int depth = 0);
};


// semantic template argument (semantic as opposed to syntactic); this
// breaks the argument down into the cases described in cppstd 14.3.2
// para 1, plus types, minus template parameters, then grouped into
// equivalence classes as implied by cppstd 14.4 para 1
class STemplateArgument {
public:
  // FIX: make these data members private
  enum Kind {
    STA_NONE,        // not yet resolved into a valid template argument
    STA_TYPE,        // type argument
    STA_INT,         // int or enum argument

    // dsw: this may not be a ref to a global object, but instead a
    // template parameter; in this example from in/t0180.cc, note the
    // use of 'C' here is an argument, where it is brought into
    // existence as a template parameter in the outer scope:
    //   template <typename A, typename B, int C>
    //   struct Traits<A, Helper1<B, Helper2<C> > >
    //
    // sm: STA_REFERENCE is being abused here; note how it would not
    // work if the code said "C+2".  We need something like STA_EXPR
    // that means "integer argument, but not evaluatable to a constant
    // in this context".
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

  // FIX: dsw: this is probably completely duplicate functionality
  // with the copy ctor, but I don't like the fast and loose bit copy
  // going on there with respect to the union
  STemplateArgument *shallowClone() const;

  // get 'value', ensuring correspondence between it and 'kind'
  Type *    getType()      const { xassert(kind==STA_TYPE);      return value.t; }
  int       getInt()       const { xassert(kind==STA_INT);       return value.i; }
  Variable *getReference() const { xassert(kind==STA_REFERENCE); return value.v; }
  Variable *getPointer()   const { xassert(kind==STA_POINTER);   return value.v; }
  Variable *getMember()    const { xassert(kind==STA_MEMBER);    return value.v; }

  // set 'value', ensuring correspondence between it and 'kind'
  void setType(Type *t)          { kind=STA_TYPE;      value.t=t; }
  void setInt(int i)             { kind=STA_INT;       value.i=i; }
  void setReference(Variable *v) { kind=STA_REFERENCE; value.v=v; }
  void setPointer(Variable *v)   { kind=STA_POINTER;   value.v=v; }
  void setMember(Variable *v)    { kind=STA_MEMBER;    value.v=v; }

  bool isObject();               // "non-type non-template" in the spec
  bool isType()                  { return kind==STA_TYPE;         }
  bool isTemplate()              { return kind==STA_TEMPLATE;     }

  bool hasValue() const { return kind!=STA_NONE; }

  // the point of boiling down the syntactic arguments into this
  // simpler semantic form is to make equality checking easy
  bool equals(STemplateArgument const *obj) const;

  // does it contain variables?
  bool containsVariables() const;

  // debug print
  string toString() const;

  // debugging
  void gdb();
  void debugPrint(int depth = 0);
};

SObjList<STemplateArgument> *cloneSArgs(SObjList<STemplateArgument> const &sargs);
string sargsToString(SObjList<STemplateArgument> const &list);
inline string sargsToString(ObjList<STemplateArgument> const &list)
  { return sargsToString((SObjList<STemplateArgument> const &)list); }


// ------------------- type factory -------------------
// The type factory is used for constructing objects that represent
// C++ types.  The reasons for using a factory instead of direct
// construction include:
//
//   - Types have a complicated and unpredictable sharing structure,
//     which makes recursive deallocation impossible.  The factory
//     is thus given responsibility for deallocation of all objects
//     created by that factory.  (Hmm.. currently there's no interface
//     for relinquishing a reference back to the factory... doh.)
//
//   - Types are intended to be immutable, and thus referentially
//     transparent.  This enables the optimization of "hash consing"
//     where multiple requests for the same equivalent object yield
//     the exact same object.  The factory is responsible for
//     maintaining the data structures necessary for this, and for
//     choosing whether to do it at all.
//
//   - It is often desirable to annotate Types, but the base Type
//     hierarchy should be free from any particular annotations.
//     The factory allows one to derive subclasses of Type to add
//     such annotations, without modifying creation sites (since
//     they use the factory).  Of course, an alternative is to use
//     a hash table on the side, but that's sometimes inconvenient.

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
  // which causes the type to be created or needed (but see below)
  virtual CVAtomicType *makeCVAtomicType(SourceLoc loc,
    AtomicType *atomic, CVFlags cv)=0;

  virtual PointerType *makePointerType(SourceLoc loc,
    CVFlags cv, Type *atType)=0;

  // this returns a Type* instead of a ReferenceType because I need to
  // be able to return an error type
  virtual Type *makeReferenceType(SourceLoc loc,
    Type *atType)=0;

  virtual FunctionType *makeFunctionType(SourceLoc loc,
    Type *retType)=0;

  virtual ArrayType *makeArrayType(SourceLoc loc,
    Type *eltType, int size = ArrayType::NO_SIZE)=0;

  virtual PointerToMemberType *makePointerToMemberType(SourceLoc loc,
    NamedAtomicType *inClassNAT, CVFlags cv, Type *atType)=0;

    
  // NOTE: I very much want to get rid of this 'loc' business in the
  // type constructors, however there is a client analysis (ccqual)
  // that currently needs them.  Once ccqual is modified to not
  // require this information, the 'loc' arguments are going to go
  // away.  Until then, it's fine to just say SL_UNKNOWN every time
  // you need to make a type.  (And in the meantime while they're
  // here, do not write code that relies on them!)


  // ---- clone types ----
  // when types are cloned, their location is expected to be copied too
  virtual CVAtomicType *cloneCVAtomicType(CVAtomicType *src)=0;
  virtual PointerType *clonePointerType(PointerType *src)=0;
  virtual ReferenceType *cloneReferenceType(ReferenceType *src)=0;
  virtual FunctionType *cloneFunctionType(FunctionType *src)=0;
  virtual ArrayType *cloneArrayType(ArrayType *src)=0;
  virtual PointerToMemberType *clonePointerToMemberType(PointerToMemberType *src)=0;
  Type *cloneType(Type *src);       // switch, clone, return

  // NOTE: all 'syntax' pointers are nullable, since there are contexts
  // where I don't have an AST node to pass

  // ---- create a type based on another one ----
  // NOTE: The functions in this section do *not* modify their argument
  // Types, rather they return a new object if the desired Type is different
  // from the one passed-in.  (That is, they behave functionally.)

  // given a type, set its cv-qualifiers to 'cv'; return NULL if the
  // base type cannot be so qualified; I pass the syntax from which
  // the 'cv' flags were derived, when I have it, for the benefit of
  // extension analyses
  virtual Type *setCVQualifiers(SourceLoc loc, CVFlags cv, Type *baseType,
                                TypeSpecifier * /*nullable*/ syntax);

  // add 'cv' to existing qualifiers; default implementation just
  // calls setCVQualifiers
  virtual Type *applyCVToType(SourceLoc loc, CVFlags cv, Type *baseType,
                              TypeSpecifier * /*nullable*/ syntax);

  // build a pointer type from a syntactic description; here I allow
  // the factory to know the name of an AST node, but the default
  // implementation will not use it, so it need not be linked in for
  // this to make sense
  //
  // returns either PointerType or ReferenceType
  virtual Type *syntaxPointerType(SourceLoc loc,
    bool isPtr, CVFlags cv, Type *underlying,
    D_pointer * /*nullable*/ syntax);

  // similar for a function type; the parameters will be added by
  // the caller after this function returns
  virtual FunctionType *syntaxFunctionType(SourceLoc loc,
    Type *retType, D_func * /*nullable*/ syntax, TranslationUnit *tunit);

  // and another for pointer-to-member
  virtual PointerToMemberType *syntaxPointerToMemberType(SourceLoc loc,
    NamedAtomicType *inClassNAT, CVFlags cv, Type *atType,
    D_ptrToMember * /*nullable*/ syntax);

  // given a class, build the type of the receiver object parameter
  // 1/19/03: it should be a *reference* type
  // 1/30/04: fixing name: it's the receiver, not 'this'
  // 4/18/04: the 'classType' has been generalized because we
  //          represent ptr-to-member-func using a FunctionType
  //          with receiver parameter of type 'classType'
  virtual Type *makeTypeOf_receiver(SourceLoc loc,
    NamedAtomicType *classType, CVFlags cv, D_func * /*nullable*/ syntax);

  // this will cause a compile error if anyone uses the old name; it
  // can be removed after the switch is complete (~ 2/29/04)
  #define makeTypeOf_this do_not_use the name makeTypeOf_this

  // given a function type and a return type, make a new function type
  // which is like it but has no parameters; i.e., copy all fields
  // except 'params'; this does *not* call doneParams
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
  // PointerType because sometimes I return ST_ERROR
  inline Type *makePtrType(SourceLoc loc, Type *type)
    { return type->isError()? type : makePointerType(loc, CV_NONE, type); }

  // map a simple type into its CVAtomicType representative
  CVAtomicType *getSimpleType(SourceLoc loc, SimpleTypeId st, CVFlags cv = CV_NONE);

  // given an array type with no size, return one that is
  // the same except its size is as specified
  ArrayType *setArraySize(SourceLoc loc, ArrayType *type, int size);
};


// This is an implementation of the above interface which returns the
// actual Type-derived objects defined, as opposed to objects of
// further-derived classes.  On the extension topics mentioned above:
//   - It does not deallocate Types at all (oh well...).
//   - It does not (yet) implement hash-consing, except for CVAtomics
//     of SimpleTypes.
//   - No annotations are added; the objects returned have exactly
//     the types declared below.
class BasicTypeFactory : public TypeFactory {
private:   // data
  // global array of non-const, non-volatile built-ins; it's expected
  // to be treated as read-only
  static CVAtomicType unqualifiedSimple[NUM_SIMPLE_TYPES];

public:    // funcs
  // TypeFactory funcs
  virtual CVAtomicType *makeCVAtomicType(SourceLoc loc, 
    AtomicType *atomic, CVFlags cv);
  virtual PointerType *makePointerType(SourceLoc loc,
    CVFlags cv, Type *atType);
  virtual Type *makeReferenceType(SourceLoc loc,
    Type *atType);
  virtual FunctionType *makeFunctionType(SourceLoc loc,
    Type *retType);
  virtual ArrayType *makeArrayType(SourceLoc loc,
    Type *eltType, int size);
  virtual PointerToMemberType *makePointerToMemberType(SourceLoc loc,
    NamedAtomicType *inClassNAT, CVFlags cv, Type *atType);

  virtual CVAtomicType *cloneCVAtomicType(CVAtomicType *src);
  virtual PointerType *clonePointerType(PointerType *src);
  virtual ReferenceType *cloneReferenceType(ReferenceType *src);
  virtual FunctionType *cloneFunctionType(FunctionType *src);
  virtual ArrayType *cloneArrayType(ArrayType *src);
  virtual PointerToMemberType *clonePointerToMemberType(PointerToMemberType *src);

  virtual Variable *makeVariable(SourceLoc L, StringRef n,
                                 Type *t, DeclFlags f, TranslationUnit *tunit);
  virtual Variable *cloneVariable(Variable *src);
};


// -------------------- XReprSize ---------------------
// thrown when the reprSize() function cannot determine an
// array size
class XReprSize : public xBase {
public:
  XReprSize();
  XReprSize(XReprSize const &obj);
  ~XReprSize();
};

void throw_XReprSize() NORETURN;


// ------ for debugging ------
// The idea here is you say "print type_toString(x)" in gdb, where 'x'
// is a pointer to some type.  The pointer that is returned will be
// printed as a string by gdb.
//
// update: it turns out the latest gdbs are capable of using the
// toString method directly!  but I'll leave this here anyway.
char *type_toString(Type const *t);


// PointerType and references:
//
// PointerType is used for both pointers and references.  This choice
// is motivated by the similarity of their run-time semantics and
// likely implementation.  The case has been made that they should be
// split into two classes, and also that ArrayType might be
// conceivably merged with PointerType, both arguments made based on
// similarities of *compile* time semantics.  So far, I haven't found
// the arguments sufficiently convincing to change the hierarchy, but
// I still consider it a possible point of future debate.
//
// UPDATE: I have now split PointerType and ReferenceType apart.
//
// One a somewhat related note, I should explain my intended
// relationship between references and lvalues.  The C++ standard uses
// the term 'reference' to refer to types (a compile-time concept),
// and 'lvalue' to refer to values (a run-time concept).  Of course, a
// reference type gives rise to an lvalue, and a (non-const) reference
// must be bound to an lvalue, so there's a close relationship between
// the two.  In constrast, the same document uses the term 'pointer'
// when referring to both types and values.
//
// In the interest of reducing the number of distinct concepts, I use
// "reference types" and "lvalues" interchangeably; non-references
// refer to rvalues.  Generally, this works well.  However, one must
// keep in mind that there is consequently a slight terminological
// difference between my stuff and the C++ standard.  For example, in
// the code
//
//   int a;
//   f(a);
//
// the argument 'a' has type 'int' and is an lvalue according to the
// standard, whereas cc_tcheck.cc just says it's an 'int &'.


#endif // CC_TYPE_H
