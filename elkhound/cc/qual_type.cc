// qual_type.cc
// code for qual_type.h

#include "qual_type.h"      // this module
#include "qual_var.h"       // Variable_Q
#include "variable.h"       // Variable
#include "cc.ast.gen.h"     // D_pointer, D_func


// ------------------------ Type_Q ----------------------
Type_Q::Type_Q(Type *corresp)
  : refType(NULL), correspType(corresp), q(NULL)
{
  // our homomorphism check is reduced to this, which simply verifies
  // my cast logic is right
  xassert(asType_Q(type()) == this);
}

Type_Q::~Type_Q()
{
  // sm: should 'q' be regarded as an owner?
}


DOWNCAST_IMPL(Type_Q, CVAtomicType_Q)
DOWNCAST_IMPL(Type_Q, PointerType_Q)
DOWNCAST_IMPL(Type_Q, FunctionType_Q)
DOWNCAST_IMPL(Type_Q, ArrayType_Q)


// ----------------------- CVAtomicType_Q ---------------------
string CVAtomicType_Q::leftString(bool innerParen) const
{
  // sm: we can come back and optimize the placement of spaces later
  stringBuilder sb;
  sb << CVAtomicType::leftString(innerParen);
  if (q) { 
    sb << ::toString(q) << " ";
  }
  return sb;
}


// ------------------------- PointerType_Q ------------------------
string PointerType_Q::leftString(bool innerParen) const
{
  return stringc << PointerType::leftString(innerParen) << ::toString(q);
}


// --------------------------- FunctionType_Q -----------------
void FunctionType_Q::addParam(Variable_Q *param)
{
  FunctionType::addParam(param);
}


string FunctionType_Q::rightStringUpToQualifiers(bool innerParen) const
{
  return stringc
    << FunctionType::rightStringUpToQualifiers(innerParen)
    << ::toString(q);
}


// ----------------------- ArrayType_Q -----------------
// nothing for now


// -------------------- TypeFactory_Q -------------------
// note that since TypeFactory_Q is a friend of all of the Type_Q
// classes, it knows they inherit from Type so we don't need to
// mess around with the type() function (and more importantly, we
// can directly *down*cast from Type to Type_Q)

TypeFactory_Q *tfactory_q = NULL;

TypeFactory_Q::TypeFactory_Q()
{                   
  if (!tfactory_q) {
    // some tolerance for multiple copies..
    tfactory_q = this;
  }
}

TypeFactory_Q::~TypeFactory_Q()
{
  if (tfactory_q == this) {
    tfactory_q = NULL;
  }
}


CVAtomicType *TypeFactory_Q::makeCVAtomicType(AtomicType *atomic, CVFlags cv)
{
  return new CVAtomicType_Q(atomic, cv);
}

PointerType *TypeFactory_Q::makePointerType(PtrOper op, CVFlags cv, Type *atType)
{
  return new PointerType_Q(op, cv, asType_Q(atType));
}

FunctionType *TypeFactory_Q::makeFunctionType(Type *retType, CVFlags cv)
{
  return new FunctionType_Q(asType_Q(retType), cv);
}

ArrayType *TypeFactory_Q::makeArrayType(Type *eltType, int size)
{
  if (size == -1) {
    return new ArrayType_Q(asType_Q(eltType));    
  }
  else {
    return new ArrayType_Q(asType_Q(eltType), size);
  }
}


#define CLONE_WRAP(type)                         \
type *TypeFactory_Q::clone##type(type *src)      \
  { return clone##type##_Q(as##type##_Q(src)); }
  
CLONE_WRAP(CVAtomicType)
CLONE_WRAP(PointerType)
CLONE_WRAP(FunctionType)
CLONE_WRAP(ArrayType)
                
#undef CLONE_WRAP


CVAtomicType_Q *TypeFactory_Q::cloneCVAtomicType_Q(CVAtomicType_Q *src)
{
  // NOTE: We don't clone the underlying AtomicType.
  CVAtomicType_Q *ret = new CVAtomicType_Q(src->atomic(), src->cv());
  ret->q = src->q ? deepCloneLiterals(src->q) : NULL;
  return ret;
}

PointerType_Q *TypeFactory_Q::clonePointerType_Q(PointerType_Q *src)
{
  PointerType_Q *ret = new PointerType_Q(src->op(), src->cv(),
                                         cloneType_Q(src->atType()));
  ret->q = src->q ? deepCloneLiterals(src->q) : NULL;
  return ret;
}

FunctionType_Q *TypeFactory_Q::cloneFunctionType_Q(FunctionType_Q *src)
{
  FunctionType_Q *ret = new FunctionType_Q(cloneType_Q(src->retType()),
                                           src->cv());
  for (SObjListIterNC<Variable_Q> param_iter(src->params());
       !param_iter.isDone(); param_iter.adv()) {
    ret->addParam(cloneVariable_Q(param_iter.data()));
//      xassert(param_iter.data()->type ==
//              param_iter.data()->decl->type // the variable
//              );
  }
  ret->setAcceptsVarargs(src->acceptsVarargs());
  // FIX: omit these for now.
//    ret->exnSpec = exnSpec->deepClone();
//    ret->templateParams = templateParams->deepClone();

  ret->q = src->q ? deepCloneLiterals(src->q) : NULL;
  return ret;
}

ArrayType_Q *TypeFactory_Q::cloneArrayType_Q(ArrayType_Q *src)
{
  ArrayType_Q *ret;
  if (src->hasSize()) {
    ret = new ArrayType_Q(cloneType_Q(src->eltType()), src->size());
  }
  else {
    ret = new ArrayType_Q(cloneType_Q(src->eltType()));
  }
  ret->q = src->q ? deepCloneLiterals(src->q) : NULL;
  return ret;
}


Type_Q *TypeFactory_Q::cloneType_Q(Type_Q *src)
{
  switch (src->getTag_Q()) {
    default: xfailure("bad type tag");
    case Type_Q::T_ATOMIC_Q:    return cloneCVAtomicType_Q(src->asCVAtomicType_Q());
    case Type_Q::T_POINTER_Q:   return clonePointerType_Q(src->asPointerType_Q());
    case Type_Q::T_FUNCTION_Q:  return cloneFunctionType_Q(src->asFunctionType_Q());
    case Type_Q::T_ARRAY_Q:     return cloneArrayType_Q(src->asArrayType_Q());
  }
}


// This is an attempt to imitate applyCVToType() for our general
// qualifiers.  Yes, there is a lot of commented-out code so the
// context of applyCVToType() is present.
Type *TypeFactory_Q::applyCVToType(
  CVFlags cv, Type *baseType, TypeSpecifier *syntax)
{
  if (baseType->isError()) {
    return baseType;
  }

  // apply 'cv' the same way the default factory does
  baseType = TypeFactory::applyCVToType(cv, baseType, syntax);
  Type_Q *qbaseType = asType_Q(baseType);

  // extract qualifier literals from syntax
  // FIX: This should be literals
  Qualifiers *q = syntax->q;

  if (!q || !q->ql) {
    // keep what we've got
    return baseType;
  }

  // first, check for special cases
  switch (qbaseType->getTag_Q()) {
  case Type_Q::T_ATOMIC_Q: {
    CVAtomicType_Q *atomic = qbaseType->asCVAtomicType_Q();
    // dsw: It seems to me that this is an optimization, so I turn
    // it off for now.
    //        if ((atomic.cv | cv) == atomic.cv) {
    //          // the given type already contains 'cv' as a subset,
    //          // so no modification is necessary
    //          return baseType;
    //        }
    //        else {
    // we have to add another CV, so that means creating
    // a new CVAtomicType_Q with the same AtomicType_Q as 'baseType'
    CVAtomicType_Q *ret = cloneCVAtomicType_Q(atomic);

    // but with the new flags added
    //          ret->cv = (CVFlags)(ret->cv | cv);

    // this already done by the constructor
    //          if (atomic.ql) ret->ql = atomic.ql->deepClone(NULL);
    //          else xassert(!ret->ql);

    // dsw: FIX: this is being applied in a way with typedefs as
    // to doubly-annotate.  Scott says this is fixed.  CHECK.
    //          cout << "OLD: " << (ret->ql?ret->ql->toString():string("(null)")) << endl;
    //          cout << "BEING APPLIED: " << ql->toString() << endl;

    // FIX: This should be replaced by applying the qualifierliterals
    // to the type_Q.  DON'T CLONE AGAIN.
    if (q) ret->q = Qualifiers_deepClone(q, ret->q);
    //          // find the end of the list and copy the QualifierLiterals there
    //          QualifierLiterals **qlp;
    //          for(qlp = &(ret->ql); *qlp; qlp = &((*qlp)->next)) {}
    //          *qlp = ql;

    return ret->type();
    //        }
    break;
  }

  case Type_Q::T_POINTER_Q: {
    // logic here is nearly identical to the T_ATOMIC case
    PointerType_Q *ptr = qbaseType->asPointerType_Q();
    if (ptr->op() == PO_REFERENCE) {
      return NULL;     // can't apply CV to references
    }
    // dsw: again, I think this is an optimization
    //        if ((ptr.cv | cv) == ptr.cv) {
    //          return baseType;
    //        }
    //        else {
    PointerType_Q *ret = clonePointerType_Q(ptr);

    //          ret->cv = (CVFlags)(ret->cv | cv);

    // this already done by the constructor
    //          if (ptr.ql) ret->ql = ptr.ql->deepClone(NULL);
    //          else xassert(!ret->ql);
    if (q) ret->q = Qualifiers_deepClone(q, ret->q);

    //          // find the end of the list and copy the QualifierLiterals there
    //          QualifierLiterals **qlp;
    //          for(qlp = &(ret->ql); *qlp; qlp = &((*qlp)->next)) {}
    //          *qlp = ql;

    return ret->type();
    //        }
    break;
  }

  case Type_Q::T_FUNCTION_Q: // FIX: Should we do anything in this case?
    return NULL; break;

  case Type_Q::T_ARRAY_Q:
    // can't apply CV to either of these (function has CV, but
    // can't get it after the fact)
    return NULL; break;

  default:    // silence warning
    xassert(0); return NULL; break;
  }
}


Type *TypeFactory_Q::makeRefType(Type *_underlying)
{
  Type_Q *underlying = asType_Q(_underlying);

  if (underlying->isError()) {
    return _underlying;
  }

  // use the 'refType' pointer so multiple requests for the
  // "reference to" a given type always yield the same object
  if (!underlying->refType) {
    underlying->refType = asPointerType_Q(
      makePointerType(PO_REFERENCE, CV_NONE, _underlying));
  }
  return underlying->refType;
}


PointerType *TypeFactory_Q::syntaxPointerType(
  PtrOper op, CVFlags cv, Type *type, D_pointer *syntax)
{
  PointerType_Q *ptq = asPointerType_Q(makePointerType(op, cv, type));
  
  // attach the qualifier literal
  ptq->q = deepClone(syntax->q);
  
  return ptq;
}


FunctionType *TypeFactory_Q::syntaxFunctionType(
  Type *retType, CVFlags cv, D_func *syntax)
{
  FunctionType_Q *ftq = asFunctionType_Q(makeFunctionType(retType, cv));
  
  // attach qualifier literal
  ftq->q = deepClone(syntax->q);

  return ftq;
}


PointerType *TypeFactory_Q::makeTypeOf_this(
  CompoundType *classType, FunctionType *methodType)
{
  CVAtomicType_Q *tmpcvat = asCVAtomicType_Q(
    makeCVAtomicType(classType, methodType->cv));
  xassert(!tmpcvat->q);
  tmpcvat->q = deepClone(asFunctionType_Q(methodType)->q);
  return makePointerType(PO_POINTER, CV_CONST, tmpcvat);
}


FunctionType *TypeFactory_Q::makeSimilarFunctionType(
  Type *retType, FunctionType *similar)
{
  FunctionType_Q *ft = asFunctionType_Q(makeFunctionType(retType, similar->cv));

//    StringRef name = "__unamed_function";
//    {
//      PQName const * declaratorId = getDeclaratorId();
//      if (declaratorId) name = declaratorId->getName();
//    }
  xassert(!ft->q);
  ft->q = deepClone(asFunctionType_Q(similar)->q);

  return ft;
}

  
Variable *TypeFactory_Q::makeVariable(SourceLocation const &L, StringRef n,
                                      Type *t, DeclFlags f)
{
  return new Variable_Q(L, n, asType_Q(t), f);
}


Variable *TypeFactory_Q::cloneVariable(Variable *src)
{
  return cloneVariable_Q(asVariable_Q(src));
}

Variable_Q *TypeFactory_Q::cloneVariable_Q(Variable_Q *src)
{
  Variable_Q *ret = new Variable_Q(src->loc
                                   ,src->name // don't see a reason to clone the name
                                   ,cloneType_Q(src->type())
                                   ,src->flags // an enum, so nothing to clone
                                   );
  // ret->overload left as NULL as set by the ctor; not cloned
  ret->access = src->access;         // an enum, so nothing to clone
  ret->scope = src->scope;           // don't see a reason to clone the scope
  return ret;
}
