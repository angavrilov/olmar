// qual_type.cc
// code for qual_type.h

#include "qual_type.h"      // this module
#include "qual_var.h"       // Variable_Q
#include "variable.h"       // Variable


// ------------------------ Type_Q ----------------------
Type_Q::Type_Q(Type const *corresp)
  : refType(NULL), correspType(corresp), q(NULL)
{}

Type_Q::Type_Q(Type_Q const &obj)
  : refType(NULL), correspType(obj.correspType), q(::deepClone(q)) 
{}

Type_Q::~Type_Q()
{
  // sm: should 'q' be regarded as an owner?
}


DOWNCAST_IMPL(Type_Q, CVAtomicType_Q)
DOWNCAST_IMPL(Type_Q, PointerType_Q)
DOWNCAST_IMPL(Type_Q, FunctionType_Q)
DOWNCAST_IMPL(Type_Q, ArrayType_Q)


Type_Q *Type_Q::getRefTypeTo()
{
  if (!refType) {
    refType = new PointerType_Q(
                // C++ type that means 'reference to <this>'
                new PointerType(PO_REFERENCE, CV_NONE, type()),
                // Q++Qual type meaning <this>
                this);
  }
  return refType;
}


string Type_Q::toCString() const
{
  return stringc << leftString() << rightString();
}

string Type_Q::toCString(char const *name) const
{
  // print the inner parentheses if the name is omitted
  bool innerParen = (name && name[0])? false : true;

  return stringc << leftString(innerParen) 
                 << (name? name : "/*anon*/")
                 << rightString(innerParen);
}


Type_Q *Type_Q::applyCV(CVFlags cv)
{
  // should be easy: apply 'cv' to the C++ type and replace
  // my 'correspType' with that one
  Type const *newType = applyCVToType(cv, correspType);
  xassert(newType);
  if (newType == correspType) {
    return this;    // common case
  }

  // if the correspType changes, I need to make a new Type_Q,
  // and the easiest way to do that is to clone it (if I don't
  // make a new one, then I'd be polluting all uses of a given
  // typedef the moment anyone wants a const version of it)
  Type_Q *ret = deepClone();
  ret->correspType = newType;

  // make sure I didn't screw up
  ret->checkHomomorphism();
  
  return ret;
}


// ----------------------- CVAtomicType_Q ---------------------
CVAtomicType_Q *CVAtomicType_Q::deepClone() const
{
  // NOTE: We don't clone the underlying AtomicType.
  CVAtomicType_Q *tmp = new CVAtomicType_Q(type());
  tmp->q = q ? ::deepCloneLiterals(q) : NULL;
  return tmp;
}


void CVAtomicType_Q::checkHomomorphism() const
{
  // nothing to check for leaves
}


string CVAtomicType_Q::leftString(bool innerParen) const
{                                                                          
  // sm: we can come back and optimize the placement of spaces later
  return stringc << type()->leftString(innerParen) << ::toString(q) << " ";
}


string CVAtomicType_Q::rightString(bool innerParen) const
{
  return type()->rightString(innerParen);
}


// ------------------------- PointerType_Q ------------------------
// local invariants only (things checkable in O(1))
void PointerType_Q::checkInvars() const
{
  // check commutativity
  // I'd been using == but that doesn't seem to hold and it's
  // not entirely clear why..
  xassert(atType->type()->equals(type()->atType));
}


PointerType_Q *PointerType_Q::deepClone() const
{
  PointerType_Q *tmp = new PointerType_Q(type(), atType->deepClone());
  tmp->q = q ? ::deepCloneLiterals(q) : NULL;
  return tmp;
}


void PointerType_Q::checkHomomorphism() const
{
  checkInvars();

  // recursively check property on type subtree
  atType->checkHomomorphism();
}


string PointerType_Q::leftString(bool innerParen) const
{
  return stringc << type()->leftString(innerParen) << ::toString(q);
}

string PointerType_Q::rightString(bool innerParen) const
{
  return type()->rightString(innerParen);
}


// --------------------------- FunctionType_Q -----------------
FunctionType_Q::FunctionType_Q(FunctionType const *corresp, Type_Q *ret)
  : Type_Q(corresp), retType(ret), params(/*initially empty*/)
{
  xassert(retType->type()->equals(type()->retType));
}


FunctionType_Q::~FunctionType_Q()
{}


FunctionType_Q *FunctionType_Q::deepClone() const
{
  // clone return type
  FunctionType_Q *tmp = new FunctionType_Q(type(), retType->deepClone());

  // copy parameters (initially in reverse)
  for (SObjListIter<Variable_Q> param_iter(params);
       !param_iter.isDone(); param_iter.adv()) {
    tmp->params.prepend(param_iter.data()->deepClone());
//      xassert(param_iter.data()->type ==
//              param_iter.data()->decl->type // the variable
//              );
  }
  tmp->params.reverse();

  tmp->q = q ? ::deepCloneLiterals(q) : NULL;
  return tmp;
}


void FunctionType_Q::checkHomomorphism() const
{                         
  // check return types
  xassert(retType->type()->equals(type()->retType));
  retType->checkHomomorphism();

  // check that the parameters agree with across the homomorphism
  xassert(params.count() == type()->params.count());
  SObjListIter<Variable_Q> iter1(params);
  ObjListIter<Parameter> iter2(type()->params);
  for (; !iter1.isDone(); iter1.adv(), iter2.adv()) {
    Type const *t1 = iter1.data()->qtype->type();
    Type const *t2 = iter2.data()->decl->type;
      // if I say "t2 = iter2.data()->type" then I get a mismatch
      // whenever t2 is involved in type normalization [cppstd 8.3.5 para 3]
    xassert(t1->equals(t2));

    // also check that the parameters are themselves internally
    // homomorphic
    iter1.data()->checkHomomorphism();
  }
}


string FunctionType_Q::leftString(bool innerParen) const
{
  return type()->leftString(innerParen);
}

string FunctionType_Q::rightString(bool innerParen) const
{
  return stringc
    << type()->rightStringUpToQualifiers(innerParen)
    << ::toString(q)
    << type()->rightStringAfterQualifiers();
}


// ----------------------- ArrayType_Q -----------------
void ArrayType_Q::checkInvars() const
{
  xassert(eltType->type()->equals(type()->eltType));
}

ArrayType_Q *ArrayType_Q::deepClone() const
{
  ArrayType_Q *tmp = new ArrayType_Q(type(), eltType->deepClone());
  tmp->q = q ? ::deepCloneLiterals(q) : NULL;
  return tmp;
}


void ArrayType_Q::checkHomomorphism() const
{
  checkInvars();
  eltType->checkHomomorphism();
}


string ArrayType_Q::leftString(bool innerParen) const
{
  return type()->leftString(innerParen);
}

string ArrayType_Q::rightString(bool innerParen) const
{
  return type()->rightString(innerParen);
}


// ---------------- global type construction utils ----------------
CVAtomicType_Q *getSimpleType_Q(SimpleTypeId id, CVFlags cv)
{
  CVAtomicType const *at = makeCVType(&SimpleType::fixed[id], cv);
  return new CVAtomicType_Q(at);
}


Type_Q *buildQualifierFreeType_Q(Type const *t)
{
  switch (t->getTag()) {
    default:
      xfailure("bad type code");

    case Type::T_ATOMIC:
      return new CVAtomicType_Q(&( t->asCVAtomicTypeC() ));

    case Type::T_POINTER: {
      PointerType const *pt = &( t->asPointerTypeC() );
      return new PointerType_Q(pt, buildQualifierFreeType_Q(pt->atType));
    }

    case Type::T_FUNCTION: {
      FunctionType const *ft = &( t->asFunctionTypeC() );
      FunctionType_Q *ret = new FunctionType_Q(ft, buildQualifierFreeType_Q(ft->retType));

      // copy parameters (initially in reverse)
      FOREACH_OBJLIST(Parameter, ft->params, iter) {
        Variable *v = iter.data()->decl;

        // if this Variable hasn't been annotated, do so now
        if (!v->q) {
          v->q = new Variable_Q(v, buildQualifierFreeType_Q(v->type));
        }

        ret->params.prepend(v->q);
      }
      ret->params.reverse();

      return ret;
    }

    case Type::T_ARRAY: {
      ArrayType const *at = &( t->asArrayTypeC() );
      return new ArrayType_Q(at, buildQualifierFreeType_Q(at->eltType));
    }
  }
}
