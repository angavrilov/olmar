// cc_type.cc
// code for cc_type.h

#include "cc_type.h"    // this module

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


// ------------------ AtomicType -----------------
AtomicType::~AtomicType()
{}


// ------------------ SimpleType -----------------
string SimpleType::toString() const
{
  return simpleTypeName(type);
}


// ------------------ CompoundType -----------------
CompoundType::CompoundType(Keyword k, char const *n)
  : keyword(k),
    name(n),
    incomplete(true)
{}


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


// --------------- Type ---------------
CAST_MEMBER_IMPL(Type, CVAtomicType)
CAST_MEMBER_IMPL(Type, PointerType)
CAST_MEMBER_IMPL(Type, FunctionType)
CAST_MEMBER_IMPL(Type, ArrayType)


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


// ---------------- EnumType ------------------
EnumType::~EnumType()
{}


string EnumType::toString() const
{
  return stringc << "enum " << name;
}


// --------------------- Type ---------------------
Type::~Type()
{}


// ----------------- CVAtomicType ----------------
string cvToString(CVFlags cv)
{
  stringBuilder sb;
  if (cv & CV_CONST) {
    sb << " const";
  }
  if (cv & CV_VOLATILE) {
    sb << " volatile";
  }
  return sb;
}

string CVAtomicType::leftString() const
{
  return stringc << atomic->toString() << cvToString(cv);
}


// ------------------- PointerType ---------------
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


// -------------------- ArrayType ------------------
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
