// qual_type.cc
// code for qual_type.h

#include "qual_type.h"      // this module
#include "qual_var.h"       // Variable_Q


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
PointerType_Q *PointerType_Q::deepClone() const
{
  PointerType_Q *tmp = new PointerType_Q(type(), atType->deepClone());
  tmp->q = q ? ::deepCloneLiterals(q) : NULL;
  return tmp;
}


void PointerType_Q::checkHomomorphism() const
{                               
  // check commutativity
  xassert(atType->type() == type()->atType);
  
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
{}


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
  xassert(retType->type() == type()->retType);
  retType->checkHomomorphism();
  
  // not clear if we have to check this for parameters since they
  // are Variable_Qs and it's possible an outer iteration of
  // checkHomomorphism will get them, but I'll do it anyway
  //
  // update: since Variable_Q now has its own checkHomomorphism,
  // I think it's clear we *should* be doing this check here
  SFOREACH_OBJLIST(Variable_Q, params, iter) {
    iter.data()->checkHomomorphism();
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
ArrayType_Q *ArrayType_Q::deepClone() const
{
  ArrayType_Q *tmp = new ArrayType_Q(type(), eltType->deepClone());
  tmp->q = q ? ::deepCloneLiterals(q) : NULL;
  return tmp;
}


void ArrayType_Q::checkHomomorphism() const
{
  xassert(eltType->type() == type()->eltType);
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
