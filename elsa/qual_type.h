// qual_type.h
// type structure to parallel the cc_type.h constructed types,
// for use with C++Qual

// The essential idea is to let a client analysis (C++Qual in this
// case) have a representation of C++ types which it can manipulate
// with arbitrary semantics, and leverage the information represented
// in cc_type, while not polluting cc_type with annotations or
// burdening it with analysis semantics.

#ifndef QUAL_TYPE_H
#define QUAL_TYPE_H

#include "cc_type.h"       // Type, etc.

class Variable_Q;          // qual_var.h

// forwards in this file
class CVAtomicType_Q;
class PointerType_Q;
class FunctionType_Q;
class ArrayType_Q;
class TypeFactory_Q;


// sm: these were opaque when part of cc_type.. I'm leaving them
// opaque as I create this file from that one, but there's no
// reason they need to stay opaque since qual_type is fully aware
// of C++Qual
class Qualifiers;
string toString(Qualifiers *q);
Qualifiers *deepClone(Qualifiers *q);
Qualifiers *deepCloneLiterals(Qualifiers *q);
Qualifiers *Qualifiers_deepClone(Qualifiers *ths, Qualifiers *q);


// This is the root of the annotated, constructed type hierarchy.
//
// This does *not* inherit Type because doing so would require virtual
// inheritance back in cc_type.h, and I don't want to do that.
// Instead, the classes below inherit from their corresponding Type
// classes.
//
// Note that in all cases below we inherit from Type_Q *first*, which
// means I can directly cast from a Type* to a Type_Q* with a cast
// because the start addresses differ by a fixed offset, namely
// 'sizeof(Type_Q)'.  I'm rather cavalier about casting to Type_Q and
// its derivatives here, because the factory ensures only those
// objects are ever created.  So I think it's pretty safe to assume
// every time I have a Type derivative I actually have the
// corresponding Type_Q derivative.
class Type_Q {
private:      // data
  friend class TypeFactory_Q;
  PointerType_Q *refType; // the type which is the reference to this type, lazily constructed

protected:
  // this is a pointer to the corresponding cc_type object; it's
  // not public because it's immutable, and because we'll use a method
  // to access it which knows what type it should refer to; this is,
  // in essence, a direct implementation of the virtual inheritance
  // mechanism (but in a way that doesn't require changing cc_type.h)
  Type *correspType;

public:
  // sm: all of the constructed type nodes had a single 'q' variable
  // before, so I've moved that into Type_Q
  Qualifiers *q;

protected:    // funcs
  // the constructors are protected to enforce using the factory
  Type_Q(Type *corresp);

public:
  virtual ~Type_Q();

  // roll-my-own RTTI
  enum Tag_Q { T_ATOMIC_Q, T_POINTER_Q, T_FUNCTION_Q, T_ARRAY_Q };
  virtual Tag_Q getTag_Q() const=0;
  bool isCVAtomicType_Q() const { return getTag_Q() == T_ATOMIC_Q; }
  bool isPointerType_Q() const { return getTag_Q() == T_POINTER_Q; }
  bool isFunctionType_Q() const { return getTag_Q() == T_FUNCTION_Q; }
  bool isArrayType_Q() const { return getTag_Q() == T_ARRAY_Q; }

  // checked downcasts
  DOWNCAST_FN(CVAtomicType_Q)
  DOWNCAST_FN(PointerType_Q)
  DOWNCAST_FN(FunctionType_Q)
  DOWNCAST_FN(ArrayType_Q)

  // This is the corresponding Type, when we don't yet know which
  // derived type it really is.
  //
  // In derived classes, we use this to cross from the Type_Q world to
  // the Type world explicitly.  The expliciteness of the crossing is
  // intended to help us if we again change the precise relationship
  // between their inheritance hierarchies.  Right now we're using
  // inheritance, but it's protected inheritance to force the use of
  // 'type()'.
  Type *type() { return correspType; }
  Type const *typeC() const { return correspType; }

  // selected parts of the Type interface, exported by delegation; there's
  // no reason not to include them all, but as an experiment I'm just
  // adding them as I need them
  string toCString() const                 { return typeC()->toCString(); }
  string toCString(char const *name) const { return typeC()->toCString(name); }
  string toString() const                  { return typeC()->toCString(); }
  bool isError() const                     { return typeC()->isError(); }
  bool isReference() const                 { return typeC()->isReference(); }

  // get associated qualifiers
  Qualifiers *&getQualifiersPtr() { return q; }
};


// a leaf of a constructed type tree
class CVAtomicType_Q : public Type_Q, protected CVAtomicType {
protected:
  friend class TypeFactory_Q;
  CVAtomicType_Q(AtomicType *a, CVFlags c)
    : Type_Q(this), CVAtomicType(a, c) {}

public:
  CVAtomicType *type() { return this; }
  CVAtomicType const *typeC() const { return this; }

  // accessors to CVAtomicType (note that since they have the same
  // name as the corresponding data members, they hide the latter)
  AtomicType *atomic() { return CVAtomicType::atomic; }
  CVFlags cv() { return CVAtomicType::cv; }

  // CVAtomicType funcs
  virtual string leftString(bool innerParen) const;

  // Type_Q funcs
  virtual Tag_Q getTag_Q() const { return T_ATOMIC_Q; }
};

// I use this to generically case from Type to Type_Q, so it would be
// possible to implement a debug version with more checking.  I put it
// below CVAtomicType_Q so I could use that as the prototypical object
// into which the Type_Q is embedded, on the assumption that the
// offsets are the same for all Type_Q derivatives.
inline Type_Q const *asTypeC_Q(Type const *t)
  { return static_cast<Type_Q const*>(static_cast<CVAtomicType_Q const*>(t)); }
inline Type_Q *asType_Q(Type *t)
  { return const_cast<Type_Q*>(asTypeC_Q(t)); }

inline CVAtomicType_Q *asCVAtomicType_Q(CVAtomicType *t)
  { return static_cast<CVAtomicType_Q*>(t); }


// pointer to something
class PointerType_Q : public Type_Q, protected PointerType {
protected:
  friend class TypeFactory_Q;
  PointerType_Q(PtrOper o, CVFlags c, Type_Q *at)
    : Type_Q(this), PointerType(o, c, at->type()) {}

public:
  PointerType *type() { return this; }
  PointerType const *typeC() const { return this; }

  // accessors to PoinerType members
  PtrOper op() { return PointerType::op; }
  CVFlags cv() { return PointerType::cv; }
  Type_Q *atType() { return asType_Q(PointerType::atType); }

  // PointerType funcs
  virtual string leftString(bool innerParen) const;

  // Type_Q funcs
  virtual Tag_Q getTag_Q() const { return T_POINTER_Q; }
};

inline PointerType_Q *asPointerType_Q(PointerType *t)
  { return static_cast<PointerType_Q*>(t); }


// function; the only node in the constructed types with
// a branching factor > 1
class FunctionType_Q : public Type_Q, protected FunctionType {
protected:
  friend class TypeFactory_Q;
  FunctionType_Q(Type_Q *retType, CVFlags cv)
    : Type_Q(this), FunctionType(retType->type(), cv) {}

public:
  FunctionType *type() { return this; }
  FunctionType const *typeC() const { return this; }

  // continuation of constructor, essentially
  void addParam(Variable_Q *param);
  void setAcceptsVarargs(bool s) { FunctionType::acceptsVarargs = s; }

  // accessors to FunctionType members
  Type_Q *retType() { return asType_Q(FunctionType::retType); }
  CVFlags cv() { return FunctionType::cv; }
  SObjList<Variable_Q> &params() {
    // this one's a little tricky because it relies on the elements of
    // FunctionType::params actually being Variable_Q pointers, and the
    // fact that the underlying representation of SObjList is independent
    // of the type of the element pointers
    return reinterpret_cast<SObjList<Variable_Q>&>(FunctionType::params);
  }
  bool acceptsVarargs() { return FunctionType::acceptsVarargs; }

  // these two are a bit nonideal because their return type internally
  // refers to Type, not Type_Q; for now it won't be a problem, and
  // the solution (if/when we need it) is straightforward: either add
  // ExnSpec and TemplateParams to the list of things made by the
  // factory, or else provide direct access to their members as
  // Type_Qs (since they are in fact Type_Q objects, thanks to the
  // factory)
  ExnSpec *exnSpec() { return FunctionType::exnSpec; }
  TemplateParams *templateParams() { return FunctionType::templateParams; }

  // FunctionType funcs
  virtual string rightStringUpToQualifiers(bool innerParen) const;

  // Type_Q funcs
  virtual Tag_Q getTag_Q() const { return T_FUNCTION_Q; }
};

inline FunctionType_Q *asFunctionType_Q(FunctionType *t)
  { return static_cast<FunctionType_Q*>(t); }


// array
class ArrayType_Q : public Type_Q, protected ArrayType {
protected:
  friend class TypeFactory_Q;
  ArrayType_Q(Type_Q *elt, int sz)
    : Type_Q(this), ArrayType(elt->type(), sz) {}

public:
  ArrayType *type() { return this; }
  ArrayType const *typeC() const { return this; }

  // access to ArrayType members
  Type_Q *eltType() { return asType_Q(ArrayType::eltType); }
  bool hasSize() { return ArrayType::hasSize(); }
  int size() { return ArrayType::size; }

  // Type_Q funcs
  virtual Tag_Q getTag_Q() const { return T_ARRAY_Q; }
};

inline ArrayType_Q *asArrayType_Q(ArrayType *t)
  { return static_cast<ArrayType_Q*>(t); }


// -------------------- TypeFactory_Q ------------------
// this is a factory to make objects in the Type_Q hierarchy, and also
// Variable_Q (conceptually in that hierarchy as well)
class TypeFactory_Q : public TypeFactory {
private:   // funcs
  CVAtomicType_Q *cloneCVAtomicType_Q(CVAtomicType_Q *src);
  PointerType_Q *clonePointerType_Q(PointerType_Q *src);
  FunctionType_Q *cloneFunctionType_Q(FunctionType_Q *src);
  ArrayType_Q *cloneArrayType_Q(ArrayType_Q *src);
  Type_Q *cloneType_Q(Type_Q *src);
  Variable_Q *cloneVariable_Q(Variable_Q *src);

public:    // funcs
  TypeFactory_Q();
  ~TypeFactory_Q();

  // TypeFactory funcs
  virtual CVAtomicType *makeCVAtomicType(AtomicType *atomic, CVFlags cv);
  virtual PointerType *makePointerType(PtrOper op, CVFlags cv, Type *atType);
  virtual FunctionType *makeFunctionType(Type *retType, CVFlags cv);
  virtual ArrayType *makeArrayType(Type *eltType, int size = ArrayType::NO_SIZE);

  virtual CVAtomicType *cloneCVAtomicType(CVAtomicType *src);
  virtual PointerType *clonePointerType(PointerType *src);
  virtual FunctionType *cloneFunctionType(FunctionType *src);
  virtual ArrayType *cloneArrayType(ArrayType *src);

  virtual Type *applyCVToType(CVFlags cv, Type *baseType, TypeSpecifier *syntax);
  virtual Type *makeRefType(Type *underlying);

  virtual PointerType *syntaxPointerType(
    PtrOper op, CVFlags cv, Type *underlying, D_pointer *syntax);
  virtual FunctionType *syntaxFunctionType(
    Type *retType, CVFlags cv, D_func *syntax);
  virtual PointerType *makeTypeOf_this(
    CompoundType *classType, FunctionType *methodType);
  virtual FunctionType *makeSimilarFunctionType(
    Type *retType, FunctionType *similar);

  virtual Variable *makeVariable(SourceLocation const &L, StringRef n,
                                 Type *t, DeclFlags f);
  virtual Variable *cloneVariable(Variable *src);
};

// let's not make it a huge pain to get ahold of one of these
extern TypeFactory_Q *tfactory_q;


#if 0
inline CVAtomicType_Q *makeType_Q(AtomicType *atomic)
  { return new CVAtomicType_Q(makeType(atomic)); }

inline Type_Q *makeRefType_Q(Type_Q *type)
  { return type->getRefTypeTo(); }

CVAtomicType_Q *getSimpleType_Q(SimpleTypeId id, CVFlags cv = CV_NONE);


// This builds a Type_Q from an arbitrary Type by recursively copying
// its structure.  Of course, the Type_Q will have no qualifiers
// anywhere inside it, so this is not used for types the user
// describes.  (In a pinch one might even use it for that purpose if
// it's certain the user type doesn't have any interesting
// qualifiers.)
Type_Q *buildQualifierFreeType_Q(Type *t);
#endif // 0


#endif // QUAL_TYPE_H

