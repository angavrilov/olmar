// cc_type.cc
// code for cc_type.h

#include "cc_type.h"    // this module
#include "cc_env.h"     // Env


bool isValid(SimpleTypeId id)
{
  return 0 <= id && id <= NUM_SIMPLE_TYPES;
}

char const *simpleTypeName(SimpleTypeId id)
{
  static char const * const arr[] = {
    "char",
    "unsigned char",
    "signed char",
    "bool",
    "int",
    "unsigned int",
    "long int",
    "unsigned long int",
    "long long",
    "unsigned long long",
    "short int",
    "unsigned short int",
    "wchar_t",
    "float",
    "double",
    "long double",
    "void"
  };
  STATIC_ASSERT(TABLESIZE(arr) == NUM_SIMPLE_TYPES);

  xassert(isValid(id));
  return arr[id];
}


int simpleTypeReprSize(SimpleTypeId id)
{
  int const arr[] = {
    1, 1, 1,              // char
    4,                    // bool
    4, 4,                 // int
    4, 4,                 // long
    8, 8,                 // long long
    2, 2,                 // short
    2,                    // wchar
    4,                    // float
    8,                    // double
    10,                   // long double
    0                     // void
  };
  STATIC_ASSERT(TABLESIZE(arr) == NUM_SIMPLE_TYPES);
  
  xassert(isValid(id));
  return arr[id];
}


// ------------------ AtomicType -----------------
AtomicType::~AtomicType()
{}


CAST_MEMBER_IMPL(AtomicType, SimpleType)
CAST_MEMBER_IMPL(AtomicType, CompoundType)
CAST_MEMBER_IMPL(AtomicType, EnumType)


bool AtomicType::equals(AtomicType const *obj) const
{
  // all of the AtomicTypes are unique-representation,
  // so pointer equality suffices
  return this == obj;
}


// ------------------ SimpleType -----------------
string SimpleType::toString() const
{
  return simpleTypeName(type);
}


int SimpleType::reprSize() const
{
  return simpleTypeReprSize(type);
}


// ------------------ CompoundType -----------------
CompoundType::CompoundType(Keyword k, char const *n)
  : keyword(k),
    name(n),
    env(NULL)
{}
 
CompoundType::~CompoundType()
{
  if (env) {
    delete env;
  }
}


void CompoundType::makeComplete(Env *parentEnv)
{
  xassert(!env);
  env = new Env(parentEnv);
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


string CompoundType::toString() const
{
  return stringc << keywordName(keyword) << " " << name;
}
  

// watch out for circular pointers (recursive types) here!
// (already bitten once ..)
string CompoundType::toStringWithFields() const
{
  stringBuilder sb;
  sb << toString();
  if (isComplete()) {
    sb << " { " << env->toString() << "};";
  }
  else {
    sb << ";";
  }
  return sb;
}


int CompoundType::reprSize() const
{
  int total = 0;
  StringObjDict<Variable>::Iter iter(env->getVariables());
  for (; !iter.isDone(); iter.next()) {
    int membSize = iter.value()->type->reprSize();
    if (keyword == K_UNION) {
      // representation size is max over field sizes
      total = max(total, membSize);
    }
    else {
      // representation size is sum over field sizes
      total += membSize;
    }
  }
  return total;
}


// ---------------- EnumType ------------------
EnumType::~EnumType()
{}


string EnumType::toString() const
{
  return stringc << "enum " << name;
}


int EnumType::reprSize() const
{
  // this is the usual choice
  return simpleTypeReprSize(ST_INT);
}


// --------------- Type ---------------
Type::~Type()
{}


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
  }
}


string Type::toString() const
{
  return stringc << leftString() << rightString();
}

string Type::toString(char const *name) const
{
  return stringc << leftString() << " " << name << rightString();
}

string Type::rightString() const
{
  return "";
}


// ----------------- CVAtomicType ----------------
bool CVAtomicType::innerEquals(CVAtomicType const *obj) const
{
  return atomic->equals(obj->atomic) &&
         cv == obj->cv;
}         


string cvToString(CVFlags cv)
{
  stringBuilder sb;
  if (cv & CV_CONST) {
    sb << " const";
  }
  if (cv & CV_VOLATILE) {
    sb << " volatile";
  }
  if (cv & CV_OWNER) {
    sb << " owner";
  }
  return sb;
}

string CVAtomicType::leftString() const
{
  return stringc << atomic->toString() << cvToString(cv);
}


int CVAtomicType::reprSize() const
{
  return atomic->reprSize();
}


// ------------------- PointerType ---------------
bool PointerType::innerEquals(PointerType const *obj) const
{
  return op == obj->op &&
         cv == obj->cv &&
         atType->equals(obj->atType);
}         


string PointerType::leftString() const
{
  return stringc << atType->leftString()
                 << (op==PO_POINTER? "*" : "&")
                 << cvToString(cv);
}

string PointerType::rightString() const
{
  return atType->rightString();
}


int PointerType::reprSize() const
{
  // a typical value ..
  return 4;
}


// -------------------- Parameter -----------------
Parameter::~Parameter()
{}
                  

string Parameter::toString() const
{
  return type->toString(name);
}


// -------------------- FunctionType -----------------
FunctionType::FunctionType(Type const *r, CVFlags c)
  : retType(r),
    cv(c),
    params(),
    acceptsVarargs(false)
{}


FunctionType::~FunctionType()
{}


bool FunctionType::innerEquals(FunctionType const *obj) const
{
  if (retType->equals(obj->retType) &&
      cv == obj->cv &&
      acceptsVarargs == obj->acceptsVarargs) {
    // so far so good, try the parameters
    ObjListIter<Parameter> iter1(params);
    ObjListIter<Parameter> iter2(obj->params);
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
  else {
    return false;
  }
}


void FunctionType::addParam(Parameter *param)
{
  params.append(param);
}


string FunctionType::leftString() const
{
  // return type and start of enclosing type's description
  return stringc << retType->leftString() << " (";
}

string FunctionType::rightString() const
{
  // finish enclosing type
  stringBuilder sb;
  sb << ")";

  // arguments
  sb << "(";
  int ct=0;
  FOREACH_OBJLIST(Parameter, params, iter) {
    if (ct++ > 0) {
      sb << ", ";
    }
    sb << iter.data()->toString();
  }
  
  if (acceptsVarargs) {
    if (ct++ > 0) {
      sb << ", ";
    }
    sb << "...";
  }

  sb << ")";

  // qualifiers
  sb << cvToString(cv);

  // finish up the return type
  sb << retType->rightString();

  return sb;
}


int FunctionType::reprSize() const
{
  // thinking here about how this works when we're summing
  // the fields of a class ...
  return 0;
}


// -------------------- ArrayType ------------------
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


string ArrayType::leftString() const
{
  return eltType->leftString();
}

string ArrayType::rightString() const
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
