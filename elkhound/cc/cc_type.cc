// cc_type.cc            see license.txt for copyright and terms of use
// code for cc_type.h

#include "cc_type.h"    // this module
#include "trace.h"      // tracingSys
#include "variable.h"   // Variable
#include "strutil.h"    // copyToStaticBuffer

#include <assert.h>     // assert

// sm: Why was this added?  I'm commenting it out because I don't see
// any calls to string.h functions.  If you put it back, include a
// comment saying which function's prototype was needed.
//#include <string.h>


string makeIdComment(int id)
{
  if (tracingSys("type-ids")) {
    return stringc << "/""*" << id << "*/";
                     // ^^ this is to work around an Emacs highlighting bug
  }
  else {
    return "";
  }
}


// ------------------ AtomicType -----------------
ALLOC_STATS_DEFINE(AtomicType)

AtomicType::AtomicType()
{
  ALLOC_STATS_IN_CTOR
}


AtomicType::~AtomicType()
{
  ALLOC_STATS_IN_DTOR
}


CAST_MEMBER_IMPL(AtomicType, SimpleType)
CAST_MEMBER_IMPL(AtomicType, CompoundType)
CAST_MEMBER_IMPL(AtomicType, EnumType)
CAST_MEMBER_IMPL(AtomicType, TypeVariable)


bool AtomicType::equals(AtomicType const *obj) const
{
  // one exception to the below: when checking for
  // equality, TypeVariables are all equivalent
  if (this->isTypeVariable() &&
      obj->isTypeVariable()) {
    return true;
  }

  // all of the AtomicTypes are unique-representation,
  // so pointer equality suffices
  //
  // it is *important* that we don't do structural equality
  // here, because then we'd be confused by types with the
  // same name that appear in different scopes!
  return this == obj;
}


// ------------------ SimpleType -----------------
SimpleType SimpleType::fixed[NUM_SIMPLE_TYPES] = {
  SimpleType(ST_CHAR),
  SimpleType(ST_UNSIGNED_CHAR),
  SimpleType(ST_SIGNED_CHAR),
  SimpleType(ST_BOOL),
  SimpleType(ST_INT),
  SimpleType(ST_UNSIGNED_INT),
  SimpleType(ST_LONG_INT),
  SimpleType(ST_UNSIGNED_LONG_INT),
  SimpleType(ST_LONG_LONG),
  SimpleType(ST_UNSIGNED_LONG_LONG),
  SimpleType(ST_SHORT_INT),
  SimpleType(ST_UNSIGNED_SHORT_INT),
  SimpleType(ST_WCHAR_T),
  SimpleType(ST_FLOAT),
  SimpleType(ST_DOUBLE),
  SimpleType(ST_LONG_DOUBLE),
  SimpleType(ST_VOID),
  SimpleType(ST_ELLIPSIS),
  SimpleType(ST_CDTOR),
  SimpleType(ST_ERROR),
  SimpleType(ST_DEPENDENT),
};

string SimpleType::toCString() const
{
  return simpleTypeName(type);
}


int SimpleType::reprSize() const
{
  return simpleTypeReprSize(type);
}


// ------------------ NamedAtomicType --------------------
NamedAtomicType::NamedAtomicType(StringRef n)
  : name(n),
    typedefVar(NULL),
    access(AK_PUBLIC)
{}

NamedAtomicType::~NamedAtomicType()
{
  if (typedefVar) {
    delete typedefVar;
  }
}


// ------------------ CompoundType -----------------
CompoundType::CompoundType(Keyword k, StringRef n)
  : NamedAtomicType(n),
    Scope(0 /*changeCount*/, SourceLocation() /*dummy loc*/),
    forward(true),
    keyword(k),
    bases(),
    templateInfo(NULL),
    syntax(NULL)
{
  curCompound = this;
  curAccess = (k==K_CLASS? AK_PRIVATE : AK_PUBLIC);
}

CompoundType::~CompoundType()
{
  bases.deleteAll();
  if (templateInfo) {
    delete templateInfo;
  }
}


STATICDEF char const *CompoundType::keywordName(Keyword k)
{
  switch (k) {
    default:          xfailure("bad keyword");
    case K_STRUCT:    return "struct";
    case K_CLASS:     return "class";
    case K_UNION:     return "union";
  }
}


string CompoundType::toCString() const
{
  stringBuilder sb;
  
  if (templateInfo) {
    sb << templateInfo->toString() << " ";
  }

  sb << keywordName(keyword) << " "
     << (name? name : "/*anonymous*/");
     
  if (templateInfo && templateInfo->specialArguments) {
    sb << "<" << templateInfo->specialArgumentsRepr << ">";
  }
   
  return sb;
}


int CompoundType::reprSize() const
{
  int total = 0;
  for (StringSObjDict<Variable>::IterC iter(getVariableIter());
       !iter.isDone(); iter.next()) {
    Variable *v = iter.value();
    // count nonstatic data members
    if (!v->type->isFunctionType() &&
        !v->hasFlag(DF_TYPEDEF) &&
        !v->hasFlag(DF_STATIC)) {
      int membSize = v->type->reprSize();
      if (keyword == K_UNION) {
        // representation size is max over field sizes
        total = max(total, membSize);
      }
      else {
        // representation size is sum over field sizes
        total += membSize;
      }
    }
  }
  return total;
}


int CompoundType::numFields() const
{                                                
  int ct = 0;
  for (StringSObjDict<Variable>::IterC iter(getVariableIter());
       !iter.isDone(); iter.next()) {
    Variable *v = iter.value();
           
    // count nonstatic data members
    if (!v->type->isFunctionType() &&
        !v->hasFlag(DF_TYPEDEF) &&
        !v->hasFlag(DF_STATIC)) {
      ct++;                        
    }
  }

  return ct;
}


void CompoundType::addField(Variable *v)
{
  addVariable(v);
}


string toString(CompoundType::Keyword k)
{
  xassert((unsigned)k < (unsigned)CompoundType::NUM_KEYWORDS);
  return string(typeIntrNames[k]);    // see cc_type.h
}


bool CompoundType::hasVirtualBase(CompoundType const *ct) const
{
  FOREACH_OBJLIST(BaseClass, bases, iter) {
    BaseClass const *b = iter.data();
    
    // is this a virtual base?
    if (b->ct == ct && b->isVirtual) {
      return true;
    }                         
    
    // does it have a virtual base?
    if (b->ct->hasVirtualBase(ct)) {
      return true;
    }
  }
  return false;
}


// ---------------- EnumType ------------------
EnumType::~EnumType()
{}


string EnumType::toCString() const
{
  return stringc << "enum " << (name? name : "/*anonymous*/");
}


int EnumType::reprSize() const
{
  // this is the usual choice
  return simpleTypeReprSize(ST_INT);
}


EnumType::Value *EnumType::addValue(StringRef name, int value, Variable *decl)
{
  xassert(!valueIndex.isMapped(name));

  Value *v = new Value(name, this, value, decl);
  values.append(v);
  valueIndex.add(name, v);
  
  return v;
}


EnumType::Value const *EnumType::getValue(StringRef name) const
{
  Value *v;
  if (valueIndex.query(name, v)) { 
    return v;
  }
  else {
    return NULL;
  }
}


// ---------------- EnumType::Value --------------
EnumType::Value::Value(StringRef n, EnumType *t, int v, Variable *d)
  : name(n), type(t), value(v), decl(d)
{}

EnumType::Value::~Value()
{}


// ------------------ TypeVariable ----------------
TypeVariable::~TypeVariable()
{}

string TypeVariable::toCString() const
{
  // use the "typename" syntax instead of "class", to distinguish
  // this from an ordinary class, and because it's syntax which
  // more properly suggests the ability to take on *any* type,
  // not just those of classes
  //
  // but, the 'typename' syntax can only be used in some specialized
  // circumstances.. so I'll suppress it in the general case and add
  // it explicitly when printing the few constructs that allow it
  //return stringc << "/*typename*/ " << name;
  return string(name);
}

int TypeVariable::reprSize() const
{
  //xfailure("you can't ask a type variable for its size");

  // this happens when we're typechecking a template class, without
  // instantiating it, and we want to verify that some size expression
  // is constant.. so make up a number
  return 4;
}


// -------------------- Type ----------------------
ALLOC_STATS_DEFINE(Type)

Type::Type()
  : refType(NULL)
{
  ALLOC_STATS_IN_CTOR
}

// FIX dsw: garbage?: should we destruct the refType?
Type::~Type()
{
  ALLOC_STATS_IN_DTOR
}


CAST_MEMBER_IMPL(Type, CVAtomicType)
CAST_MEMBER_IMPL(Type, PointerType)
CAST_MEMBER_IMPL(Type, FunctionType)
CAST_MEMBER_IMPL(Type, ArrayType)


bool Type::equals(Type const *obj) const
{
  if (getTag() != obj->getTag()) {
    return false;
  }

  switch (getTag()) {
    default: xfailure("bad tag");
    #define C(tag,type) \
      case tag: return ((type const*)this)->innerEquals((type const*)obj);
    C(T_ATOMIC, CVAtomicType)
    C(T_POINTER, PointerType)
    C(T_FUNCTION, FunctionType)
    C(T_ARRAY, ArrayType)
    #undef C
  }
}


string Type::idComment() const
{
  return makeIdComment(getId());
}


string cvToString(CVFlags cv)
{
  if (cv != CV_NONE) {
    return stringc << " " << toString(cv);
  }
  else {
    return string("");
  }
}


string Type::toCString() const
{
  if (isCVAtomicType()) {
    // special case a single atomic type, so as to avoid
    // printing an extra space
    CVAtomicType const &atomic = asCVAtomicTypeC();
    return stringc
      << atomic.atomic->toCString()
      << ::toString(atomic.q)
      << cvToString(atomic.cv);
  }
  else {
    return stringc << idComment() << leftString() << rightString();
  }
}

string Type::toCString(char const *name) const
{
  // print the inner parentheses if the name is omitted
  bool innerParen = (name && name[0])? false : true;

  #if 0    // wrong
  // except, if this type is a pointer, then omit the parens anyway;
  // we only need parens when the type is a function or array and
  // the name is missing
  if (isPointerType()) {
    innerParen = false;
  }                
  #endif // 0

  stringBuilder s;
  s << idComment();
  s << leftString(innerParen);
  s << (name? name : "/*anon*/");
  s << rightString(innerParen);
  return s;
}

string Type::rightString(bool /*innerParen*/) const
{
  return "";
}


bool Type::isSimpleType() const
{
  if (isCVAtomicType()) {
    AtomicType const *at = asCVAtomicTypeC().atomic;
    return at->isSimpleType();
  }
  else {
    return false;
  }
}

SimpleType const &Type::asSimpleTypeC() const
{
  return asCVAtomicTypeC().atomic->asSimpleTypeC();
}

bool Type::isSimple(SimpleTypeId id) const
{
  return isSimpleType() &&
         asSimpleTypeC().type == id;
}

bool Type::isIntegerType() const
{
  return isSimpleType() &&
         simpleTypeInfo(asSimpleTypeC().type).isInteger;
}


bool Type::isCompoundTypeOf(CompoundType::Keyword keyword) const
{
  if (isCVAtomicType()) {
    CVAtomicType const &a = asCVAtomicTypeC();
    return a.atomic->isCompoundType() &&
           a.atomic->asCompoundTypeC().keyword == keyword;
  }
  else {
    return false;
  }
}

CompoundType *Type::ifCompoundType()
{
  return isCompoundType()? asCompoundType() : NULL;
}

CompoundType const *Type::asCompoundTypeC() const
{
  return &( asCVAtomicTypeC().atomic->asCompoundTypeC() );
}

bool Type::isOwnerPtr() const
{
  return isPointer() && ((asPointerTypeC().cv & CV_OWNER) != 0);
}

bool Type::isPointer() const
{
  return isPointerType() && asPointerTypeC().op == PO_POINTER;
}

bool Type::isReference() const
{
  return isPointerType() && asPointerTypeC().op == PO_REFERENCE;
}

Type *Type::asRval()
{
  if (isReference()) {
    // note that due to the restriction about stacking reference
    // types, unrolling more than once is never necessary
    return asPointerType().atType;
  }
  else {
    return this;
  }
}


bool Type::isCVAtomicType(AtomicType::Tag tag) const
{
  return isCVAtomicType() &&
         asCVAtomicTypeC().atomic->getTag() == tag;
}

bool Type::isTemplateFunction() const
{
  return isFunctionType() &&
         asFunctionTypeC().isTemplate();
}

bool Type::isTemplateClass() const
{
  return isCompoundType() &&
         asCompoundTypeC()->isTemplate();
}

bool Type::isCDtorFunction() const
{
  return isFunctionType() &&
         asFunctionTypeC().retType->isSimple(ST_CDTOR);
}


bool typeIsError(Type const *t)
{
  return t->isError();
}

bool Type::containsErrors() const
{
  return anyCtorSatisfies(typeIsError);
}


bool typeHasTypeVariable(Type const *t)
{
  return t->isTypeVariable() ||
         (t->isCompoundType() &&
          t->asCompoundTypeC()->isTemplate());
}

bool Type::containsTypeVariables() const
{
  return anyCtorSatisfies(typeHasTypeVariable);
}


// ----------------- CVAtomicType ----------------
bool CVAtomicType::innerEquals(CVAtomicType const *obj) const
{
  return atomic->equals(obj->atomic) &&
         cv == obj->cv;
}


string CVAtomicType::atomicIdComment() const
{
  return makeIdComment(atomic->getId());
}


string CVAtomicType::leftString(bool /*innerParen*/) const
{
  stringBuilder s;
  s << atomicIdComment();
  s << atomic->toCString();
  s << cvToString(cv);
  s << ::toString(q);
  
  // this is the only mandatory space in the entire syntax
  // for declarations; it separates the type specifier from
  // the declarator(s)
  s << " ";

  return s;
}


int CVAtomicType::reprSize() const
{
  return atomic->reprSize();
}


bool CVAtomicType::anyCtorSatisfies(TypePred pred) const
{
  return pred(this);
}


// ------------------- PointerType ---------------
PointerType::PointerType(PtrOper o, CVFlags c, Type *a)
  : op(o), q(NULL), cv(c), atType(a)
{
  // since references are always immutable, it makes no sense to
  // apply const or volatile to them
  xassert(op==PO_REFERENCE? cv==CV_NONE : true);

  // it also makes no sense to stack reference operators underneath
  // other indirections (i.e. no ptr-to-ref, nor ref-to-ref)
  xassert(!a->isReference());
}


bool PointerType::innerEquals(PointerType const *obj) const
{
  return op == obj->op &&
         cv == obj->cv &&
         atType->equals(obj->atType);
}


string PointerType::leftString(bool /*innerParen*/) const
{
  stringBuilder s;
  s << atType->leftString(false /*innerParen*/);
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << "(";
  }
  s << (op==PO_POINTER? "*" : "&");
  s << ::toString(q);
  s << cvToString(cv);
  return s;
}

string PointerType::rightString(bool /*innerParen*/) const
{
  stringBuilder s;
  if (atType->isFunctionType() ||
      atType->isArrayType()) {
    s << ")";
  }
  s << atType->rightString(false /*innerParen*/);
  return s;
}


int PointerType::reprSize() const
{
  // a typical value .. (architecture-dependent)
  return 4;
}


bool PointerType::anyCtorSatisfies(TypePred pred) const
{
  return pred(this) ||
         atType->anyCtorSatisfies(pred);
}


// -------------------- FunctionType::ExnSpec --------------
FunctionType::ExnSpec::~ExnSpec()
{
  types.removeAll();
}


bool FunctionType::ExnSpec::anyCtorSatisfies(Type::TypePred pred) const
{
  SFOREACH_OBJLIST(Type, types, iter) {
    if (iter.data()->anyCtorSatisfies(pred)) {
      return true;
    }
  }
  return false;
}


// -------------------- FunctionType -----------------
FunctionType::FunctionType(Type *r, CVFlags c)
  : retType(r),
    q(NULL),
    cv(c),
    params(),
    acceptsVarargs(false),
    exnSpec(NULL),
    templateParams(NULL)
{}


FunctionType::~FunctionType()
{
  if (exnSpec) {
    delete exnSpec;
  }
  if (templateParams) {
    delete templateParams;
  }
}


bool FunctionType::innerEquals(FunctionType const *obj) const
{
  if (retType->equals(obj->retType) &&
      //cv == obj->cv &&     // this is part of the parameter list
      acceptsVarargs == obj->acceptsVarargs) {
    // so far so good, try the parameters
    return equalParameterLists(obj) &&
           equalExceptionSpecs(obj);
  }
  else {
    return false;
  }
}

bool FunctionType::equalParameterLists(FunctionType const *obj) const
{
  // dsw: FIX: The comment below makes no sense to me.  If it *is*
  // legal to overload with different cv qualifiers, then why does it
  // cause us to return false?  Anyway, omitting checking for
  // QualifierLiterals here.
  
  // sm: The sense of the return value is correct.  We only allow
  // overloading when the types are different.  If this function
  // were to return true, then the type checker would consider this
  // a duplicate definition and complain.  Since this function
  // instead returns false, the type checker thinks a different
  // function (with a different type signature) is being defined
  // (or declared, but definitions are the interesting case).

  if (cv != obj->cv) {
    // the 'cv' qualifier is really attached to the 'this' parameter;
    // it's important to check this here, since disequality of
    // parameter lists is part of the criteria for allowing
    // overloading, and it *is* legal to overload with different 'cv'
    // flags on a class method
    return false;
  }

  SObjListIter<Variable> iter1(params);
  SObjListIter<Variable> iter2(obj->params);
  for (; !iter1.isDone() && !iter2.isDone();
       iter1.adv(), iter2.adv()) {
    // parameter names do not have to match, but
    // the types do
    if (iter1.data()->type->equals(iter2.data()->type)) {
      // ok
    }
    else {
      return false;
    }
  }

  return iter1.isDone() == iter2.isDone();
}


// almost identical code to the above.. list comparison is
// always a pain..
bool FunctionType::equalExceptionSpecs(FunctionType const *obj) const
{
  if (exnSpec==NULL && obj->exnSpec==NULL)  return true;
  if (exnSpec==NULL || obj->exnSpec==NULL)  return false;

  // hmm.. this is going to end up requiring that exception specs
  // be listed in the same order, whereas I think the semantics
  // imply they're more like a set.. oh well
  SObjListIter<Type> iter1(exnSpec->types);
  SObjListIter<Type> iter2(obj->exnSpec->types);
  for (; !iter1.isDone() && !iter2.isDone();
       iter1.adv(), iter2.adv()) {
    if (iter1.data()->equals(iter2.data())) {
      // ok
    }
    else {
      return false;
    }
  }

  return iter1.isDone() == iter2.isDone();
}


void FunctionType::addParam(Variable *param)
{
  params.append(param);
}


string FunctionType::leftString(bool innerParen) const
{
  stringBuilder sb;

  // template parameters
  if (templateParams) {
    sb << templateParams->toString() << " ";
  }

  // return type and start of enclosing type's description

  // NOTE: we do *not* propagate 'innerParen'!  
  sb << retType->leftString();
  if (innerParen) {
    sb << "(";
  }

  return sb;
}

string FunctionType::rightString(bool innerParen) const
{
  // I split this into two pieces because the C++Qual concrete
  // syntax puts $tainted into the middle of my rightString,
  // since it's following the placement of 'const' and 'volatile'
  return stringc
    << rightStringUpToQualifiers(innerParen)
    << rightStringAfterQualifiers();
}

string FunctionType::rightStringUpToQualifiers(bool innerParen) const
{
  // finish enclosing type
  stringBuilder sb;
  if (innerParen) {
    sb << ")";
  }

  // arguments
  sb << "(";
  int ct=0;
  SFOREACH_OBJLIST(Variable, params, iter) {
    if (ct++ > 0) {
      sb << ", ";
    }
    sb << iter.data()->toStringAsParameter();
  }

  if (acceptsVarargs) {
    if (ct++ > 0) {
      sb << ", ";
    }
    sb << "...";
  }

  sb << ")";

  // qualifiers
  sb << ::toString(q);
  if (cv) {
    sb << " " << ::toString(cv);
  }
  
  return sb;
}

string FunctionType::rightStringAfterQualifiers() const
{           
  stringBuilder sb;

  // exception specs
  if (exnSpec) {
    sb << " throw(";                
    int ct=0;
    SFOREACH_OBJLIST(Type, exnSpec->types, iter) {
      if (ct++ > 0) {
        sb << ", ";
      }
      sb << iter.data()->toString();
    }
    sb << ")";
  }

  // finish up the return type
  sb << retType->rightString();

  return sb;
}


int FunctionType::reprSize() const
{
  // thinking here about how this works when we're summing
  // the fields of a class with member functions ..
  return 0;
}


bool parameterListCtorSatisfies(Type::TypePred pred, 
                                SObjList<Variable> const &params)
{
  SFOREACH_OBJLIST(Variable, params, iter) {
    if (iter.data()->type->anyCtorSatisfies(pred)) {
      return true;
    }
  }
  return false;
}

bool FunctionType::anyCtorSatisfies(TypePred pred) const
{
  return pred(this) ||
         retType->anyCtorSatisfies(pred) ||
         parameterListCtorSatisfies(pred, params) ||
         (exnSpec && exnSpec->anyCtorSatisfies(pred)) ||
         (templateParams && templateParams->anyCtorSatisfies(pred));
}



// ----------------- TemplateParams --------------
TemplateParams::~TemplateParams()
{}


string TemplateParams::toString() const
{
  stringBuilder sb;
  sb << "template <";
  int ct=0;
  SFOREACH_OBJLIST(Variable, params, iter) {
    if (ct++ > 0) {
      sb << ", ";
    }
    sb << iter.data()->toStringAsParameter();
  }
  sb << ">";
  return sb;
}


bool TemplateParams::equalTypes(TemplateParams const *obj) const
{
  SObjListIter<Variable> iter1(params), iter2(obj->params);
  for (; !iter1.isDone() && !iter2.isDone();
       iter1.adv(), iter2.adv()) {
    if (iter1.data()->type->equals(iter2.data()->type)) {
      // ok
    }
    else {
      return false;
    }
  }

  return iter1.isDone() == iter2.isDone();
}


bool TemplateParams::anyCtorSatisfies(Type::TypePred pred) const
{
  return parameterListCtorSatisfies(pred, params);
}


// ------------------ ClassTemplateInfo -------------
ClassTemplateInfo::ClassTemplateInfo()
  : instantiated(),           // empty map
    specializations(),        // empty list
    specialArguments(NULL)    // empty list
{}

ClassTemplateInfo::~ClassTemplateInfo()
{}


// -------------------- ArrayType ------------------
void ArrayType::checkWellFormedness() const
{
  // you can't have an array of references
  xassert(!eltType->isReference());
}


bool ArrayType::innerEquals(ArrayType const *obj) const
{
  if (!( eltType->equals(obj->eltType) &&
         hasSize == obj->hasSize )) {
    return false;
  }

  if (hasSize) {
    return size == obj->size;
  }
  else {
    return true;
  }
}


string ArrayType::leftString(bool /*innerParen*/) const
{
  return eltType->leftString();
}

string ArrayType::rightString(bool /*innerParen*/) const
{
  stringBuilder sb;

  if (hasSize) {
    sb << "[" << size << "]";
  }
  else {
    sb << "[]";
  }

  sb << eltType->rightString();

  return sb;
}


int ArrayType::reprSize() const
{
  if (hasSize) {
    return eltType->reprSize() * size;
  }
  else {
    // or should I throw an exception ..?
    cout << "warning: reprSize of a sizeless array\n";
    return 0;
  }
}


bool ArrayType::anyCtorSatisfies(TypePred pred) const
{
  return pred(this) ||
         eltType->anyCtorSatisfies(pred);
}


// ---------------------- TypeFactory ---------------------
Type *TypeFactory::makeRefType(Type *underlying)
{
  if (underlying->isError()) {
    return underlying;
  }
  else {
    return makePointerType(PO_REFERENCE, CV_NONE, underlying);
  }
}


Type *TypeFactory::makePtrOperType(PtrOper op, CVFlags cv, Type *type)
{
  if (type->isError()) {
    return type;      // this is why this function doesn't return PointerType ...
  }

  return makePointerType(op, cv, type);
}


CVAtomicType *TypeFactory::getSimpleType(SimpleTypeId st, CVFlags cv)
{
  xassert((unsigned)st < (unsigned)NUM_SIMPLE_TYPES);
  return makeCVAtomicType(&SimpleType::fixed[st], cv);
}


// -------------------- BasicTypeFactory ----------------------
// this is for when I split Type from Type_Q
#if 0
CVAtomicType BasicTypeFactory::unqualifiedSimple[NUM_SIMPLE_TYPES] = {
  #define CVAT(id) \
    CVAtomicType(&SimpleType::fixed[id], CV_NONE),
  CVAT(ST_CHAR)
  CVAT(ST_UNSIGNED_CHAR)
  CVAT(ST_SIGNED_CHAR)
  CVAT(ST_BOOL)
  CVAT(ST_INT)
  CVAT(ST_UNSIGNED_INT)
  CVAT(ST_LONG_INT)
  CVAT(ST_UNSIGNED_LONG_INT)
  CVAT(ST_LONG_LONG)
  CVAT(ST_UNSIGNED_LONG_LONG)
  CVAT(ST_SHORT_INT)
  CVAT(ST_UNSIGNED_SHORT_INT)
  CVAT(ST_WCHAR_T)
  CVAT(ST_FLOAT)
  CVAT(ST_DOUBLE)
  CVAT(ST_LONG_DOUBLE)
  CVAT(ST_VOID)
  CVAT(ST_ELLIPSIS)
  CVAT(ST_CDTOR)
  CVAT(ST_ERROR)
  CVAT(ST_DEPENDENT)
  #undef CVAT
};

CVAtomicType const *getSimpleType(SimpleTypeId st)
{
  xassert((unsigned)st < (unsigned)NUM_SIMPLE_TYPES);
  return &(CVAtomicType::fixed[st]);
}
#endif // 0


CVAtomicType *BasicTypeFactory::makeCVAtomicType(AtomicType *atomic, CVFlags cv)
{
  return new CVAtomicType(atomic, cv);
}


PointerType *BasicTypeFactory::makePointerType(PtrOper op, CVFlags cv, Type *atType)
{
  return new PointerType(op, cv, atType);
}


FunctionType *BasicTypeFactory::makeFunctionType(Type *retType, CVFlags cv)
{
  return new FunctionType(retType, cv);
}


ArrayType *BasicTypeFactory::makeArrayType(Type *eltType, int size)
{                                     
  if (size == -1) {
    // unspecified size
    return new ArrayType(eltType);
  }
  else {
    return new ArrayType(eltType, size);
  }
}



Type *BasicTypeFactory::applyCVToType(CVFlags cv, Type *baseType)
{
  if (baseType->isError()) {
    return baseType;
  }

  if (cv == CV_NONE) {
    // keep what we've got
    return baseType;
  }

  // the idea is we're trying to apply 'cv' to 'baseType'; for
  // example, we could have gotten baseType like
  //   typedef unsigned char byte;     // baseType == unsigned char
  // and want to apply const:
  //   byte const b;                   // cv = CV_CONST
  // yielding final type
  //   unsigned char const             // return value from this fn

  // first, check for special cases
  switch (baseType->getTag()) {
    case Type::T_ATOMIC: {
      CVAtomicType &atomic = baseType->asCVAtomicType();
      if ((atomic.cv | cv) == atomic.cv) {
        // the given type already contains 'cv' as a subset,
        // so no modification is necessary
        return baseType;
      }
      else {
        // we have to add another CV, so that means creating
        // a new CVAtomicType with the same AtomicType as 'baseType',
        // but with the new flags added
        return makeCVAtomicType(atomic.atomic, atomic.cv | cv);
      }
      break;
    }

    case Type::T_POINTER: {
      // logic here is nearly identical to the T_ATOMIC case
      PointerType &ptr = baseType->asPointerType();
      if (ptr.op == PO_REFERENCE) {
        return NULL;     // can't apply CV to references
      }
      if ((ptr.cv | cv) == ptr.cv) {
        return baseType;
      }
      else {
        return makePointerType(ptr.op, ptr.cv | cv, ptr.atType);
      }
      break;
    }

    default:    // silence warning
    case Type::T_FUNCTION:
    case Type::T_ARRAY:
      // can't apply CV to either of these (function has CV, but
      // can't get it after the fact)
      return NULL;
  }
}


ArrayType *BasicTypeFactory::setArraySize(ArrayType *type, int size)
{
  ArrayType *ret = new ArrayType(type->eltType, size);
  return ret;
}


Type *BasicTypeFactory::makeRefType(Type *underlying)
{
  if (underlying->isError()) {
    return underlying;
  }

  // use the 'refType' pointer so multiple requests for the
  // "reference to" a given type always yield the same object
  if (!underlying->refType) {
    underlying->refType = makePointerType(PO_REFERENCE, CV_NONE, underlying);
  }
  return underlying->refType;
}


Type *BasicTypeFactory::cloneType(Type *src1)
{
  switch (src1->getTag()) {
    default: xfailure("bad tag");

    case Type::T_ATOMIC: {
      // NOTE: We don't clone the underlying AtomicType.
      CVAtomicType *src = &( src1->asCVAtomicType() );
      CVAtomicType *ret = new CVAtomicType(src->atomic, src->cv);
      ret->q = src->q ? deepCloneLiterals(src->q) : NULL;
      return ret;
    }

    case Type::T_POINTER: {
      PointerType *src = &( src1->asPointerType() );
      PointerType *ret = new PointerType(src->op, src->cv, cloneType(src->atType));
      ret->q = src->q ? deepCloneLiterals(src->q) : NULL;
      return ret;
    }

    case Type::T_FUNCTION: {
      FunctionType *src = &( src1->asFunctionType() );
      FunctionType *ret = new FunctionType(cloneType(src->retType), src->cv);
      for (SObjListIterNC<Variable> param_iter(src->params);
           !param_iter.isDone(); param_iter.adv()) {
        ret->addParam(cloneVariable(param_iter.data()));
    //      xassert(param_iter.data()->type ==
    //              param_iter.data()->decl->type // the variable
    //              );
      }
      ret->acceptsVarargs = src->acceptsVarargs;
      // FIX: omit these for now.
    //    ret->exnSpec = exnSpec->deepClone();
    //    ret->templateParams = templateParams->deepClone();

      ret->q = src->q ? deepCloneLiterals(src->q) : NULL;
      return ret;
    }

    case Type::T_ARRAY: {
      ArrayType *src = &( src1->asArrayType() );
      ArrayType *ret;
      if (src->hasSize) {
        ret = new ArrayType(cloneType(src->eltType), src->size);
      }
      else {
        ret = new ArrayType(cloneType(src->eltType));
      }
      ret->q = src->q ? deepCloneLiterals(src->q) : NULL;
      return ret;
    }
  }
}


Variable *BasicTypeFactory::makeVariable(
  SourceLocation const &L, StringRef n, Type *t, DeclFlags f)
{
  return new Variable(L, n, t, f);
}

Variable *BasicTypeFactory::cloneVariable(Variable *src)
{
  Variable *ret = new Variable(src->loc
                               ,src->name // don't see a reason to clone the name
                               ,cloneType(src->type)
                               ,src->flags // an enum, so nothing to clone
                               );
  // ret->overload left as NULL as set by the ctor; not cloned
  ret->access = src->access;         // an enum, so nothing to clone
  ret->scope = src->scope;           // don't see a reason to clone the scope
  return ret;
}


// ------------------ test -----------------
void cc_type_checker()
{
  #ifndef NDEBUG
  // verify the fixed[] arrays
  // it turns out this is probably redundant, since the compiler will
  // complain if I leave out an entry, now that the classes do not have
  // default constructors! yes!
  for (int i=0; i<NUM_SIMPLE_TYPES; i++) {
    assert(SimpleType::fixed[i].type == i);

    #if 0    // disabled for now
    SimpleType const *st = (SimpleType const*)(CVAtomicType::fixed[i].atomic);
    assert(st && st->type == i);
    #endif // 0
  }
  #endif // NDEBUG
}


// --------------- debugging ---------------
char *type_toString(Type const *t)
{
  // defined in smbase/strutil.cc
  return copyToStaticBuffer(t->toString());
}


