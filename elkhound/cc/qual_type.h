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


// sm: these were opaque when part of cc_type.. I'm leaving them
// opaque as I create this file from that one, but there's no
// reason they need to stay opaque since qual_type is fully aware
// of C++Qual
class Qualifiers;
string toString(Qualifiers *q);
Qualifiers *deepClone(Qualifiers *q);
Qualifiers *deepCloneLiterals(Qualifiers *q);


// root of the constructed type hierarchy
class Type_Q {
private:
  Type_Q *refType; // the type which is the reference to this type, lazily constructed

protected:
  // this is a pointer to the corresponding cc_type object; it's
  // not public because it's immutable, and because we'll use a method
  // to access it which knows what type it should refer to; these
  // are 'const' because the design of the Type hierarchy is that
  // Type objects aren't mutable after creation
  Type const *correspType;

public:
  // sm: all of the constructed type nodes had a single 'q' variable
  // before, so I've moved that into Type_Q
  Qualifiers *q;

public:
  Type_Q(Type const *corresp);
  Type_Q(Type_Q const &obj);
  virtual ~Type_Q();

  // roll-my-own RTTI
  enum Tag { T_ATOMIC, T_POINTER, T_FUNCTION, T_ARRAY };
  virtual Tag getTag() const=0;
  bool isCVAtomicType_Q() const { return getTag() == T_ATOMIC; }
  bool isPointerType_Q() const { return getTag() == T_POINTER; }
  bool isFunctionType_Q() const { return getTag() == T_FUNCTION; }
  bool isArrayType_Q() const { return getTag() == T_ARRAY; }

  // checked downcasts
  DOWNCAST_FN(CVAtomicType_Q)
  DOWNCAST_FN(PointerType_Q)
  DOWNCAST_FN(FunctionType_Q)
  DOWNCAST_FN(ArrayType_Q)

  // corresponding Type, when we don't yet know which derived
  // type it really is
  Type const *type() const { return correspType; }

  // clone the constructed type structure
  virtual Type_Q *deepClone() const=0;

  // get the canonical reference-to type for this one
  Type_Q *getRefTypeTo();

  // get associated qualifiers
  Qualifiers *&getQualifiersPtr() { return q; }

  // check that this structure is properly parallel with the
  // one in cc_type; in particular, whenever a type constructor
  // has children, it should be equivalent to traverse the child
  // pointers in the _Q world and in the non-_Q world; if this
  // check fails, an assertion fails
  virtual void checkHomomorphism() const=0;
                            
  // apply some 'cv' flags to the underlying C++ type; usually
  // just returns 'this'
  Type_Q *applyCV(CVFlags cv);

  // type rendering, back to its C++Qual concrete syntax
  string toCString() const;
  string toCString(char const *name) const;
  virtual string leftString(bool innerParen=true) const = 0;
  virtual string rightString(bool innerParen=true) const = 0;
};


// a leaf of a constructed type tree
class CVAtomicType_Q : public Type_Q {
public:
  CVAtomicType_Q(CVAtomicType const *corresp) : Type_Q(corresp) {}
  CVAtomicType_Q(CVAtomicType_Q const &obj)   : Type_Q(obj) {}

  CVAtomicType const *type() const
    { return static_cast<CVAtomicType const*>(correspType); }

  // Type_Q funcs
  virtual Tag getTag() const { return T_ATOMIC; }
  virtual CVAtomicType_Q *deepClone() const;
  virtual void checkHomomorphism() const;
  virtual string leftString(bool innerParen) const;
  virtual string rightString(bool innerParen) const;
};


// pointer to something
class PointerType_Q : public Type_Q {
public:
  Type_Q *atType;
  Qualifiers *q;

private:
  void checkInvars() const;

public:
  PointerType_Q(PointerType const *corresp, Type_Q *at)
    : Type_Q(corresp), atType(at) { checkInvars(); }
  PointerType_Q(PointerType_Q const &obj)
    : Type_Q(obj), atType(obj.atType) { checkInvars(); }

  PointerType const *type() const
    { return static_cast<PointerType const*>(correspType); }

  // Type_Q funcs
  virtual Tag getTag() const { return T_POINTER; }
  virtual PointerType_Q *deepClone() const;
  virtual void checkHomomorphism() const;
  virtual string leftString(bool innerParen) const;
  virtual string rightString(bool innerParen) const;
};


// function; the only node in the constructed types with
// a branching factor > 1
class FunctionType_Q : public Type_Q {
public:
  Type_Q *retType;                // return type
  SObjList<Variable_Q> params;    // parameter info, including types

public:
  FunctionType_Q(FunctionType const *corresp, Type_Q *retType);
  // sm: why no copy ctor?  don't know.  for now I'm not fixing it.
  virtual ~FunctionType_Q();

  FunctionType const *type() const
    { return static_cast<FunctionType const*>(correspType); }

  // Type_Q funcs
  virtual Tag getTag() const { return T_FUNCTION; }
  virtual FunctionType_Q *deepClone() const;
  virtual void checkHomomorphism() const;
  virtual string leftString(bool innerParen) const;
  virtual string rightString(bool innerParen) const;
};


// array
class ArrayType_Q : public Type_Q {
public:
  Type_Q *eltType;

private:
  void checkInvars() const;

public:
  ArrayType_Q(ArrayType const *corresp, Type_Q *elt)
    : Type_Q(corresp), eltType(elt) { checkInvars(); }

  ArrayType const *type() const
    { return static_cast<ArrayType const*>(correspType); }

  // Type_Q funcs
  virtual Tag getTag() const { return T_ARRAY; }
  virtual ArrayType_Q *deepClone() const;
  virtual void checkHomomorphism() const;
  virtual string leftString(bool innerParen) const;
  virtual string rightString(bool innerParen) const;
};



// --------------------- type constructors ----------------

inline CVAtomicType_Q *makeType_Q(AtomicType const *atomic)
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
Type_Q *buildQualifierFreeType_Q(Type const *t);



#endif // QUAL_TYPE_H

