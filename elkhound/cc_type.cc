// cc_type.cc
// code for cc_type.h

#include "cc_type.h"    // this module
#include "cc_env.h"     // Env
#include "trace.h"      // tracingSys
#include "cil.h"        // CilExpr


bool isValid(SimpleTypeId id)
{
  return 0 <= id && id <= NUM_SIMPLE_TYPES;
}


static SimpleTypeInfo const simpleTypeInfoArray[] = {
  //name                   size  int?
  { "char",                1,    true    },
  { "unsigned char",       1,    true    },
  { "signed char",         1,    true    },
  { "bool",                4,    true    },
  { "int",                 4,    true    },
  { "unsigned int",        4,    true    },
  { "long int",            4,    true    },
  { "unsigned long int",   4,    true    },
  { "long long",           8,    true    },
  { "unsigned long long",  8,    true    },
  { "short int",           2,    true    },
  { "unsigned short int",  2,    true    },
  { "wchar_t",             2,    true    },
  { "float",               4,    false   },
  { "double",              8,    false   },
  { "long double",         10,   false   },
  
  // gnu: sizeof(void) is 1
  { "void",                1,    false   },
};

SimpleTypeInfo const &simpleTypeInfo(SimpleTypeId id)
{
  STATIC_ASSERT(TABLESIZE(simpleTypeInfoArray) == NUM_SIMPLE_TYPES);
  xassert(isValid(id));
  return simpleTypeInfoArray[id];
}


string makeIdComment(int id)
{
  if (tracingSys("type-ids")) {
    return stringc << "/""*" << id << "*/";
  }
  else {
    return "";
  }
}


// ------------------ AtomicType -----------------
ALLOC_STATS_DEFINE(AtomicType)

AtomicType::AtomicType()
  : id(NULL_ATOMICTYPEID)
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


bool AtomicType::equals(AtomicType const *obj) const
{
  // all of the AtomicTypes are unique-representation,
  // so pointer equality suffices
  return this == obj;
}


template <class T>
string recurseCilString(T const *type, int depth)
{
  depth--;
  xassert(depth >= 0);     // otherwise we started < 1
  if (depth == 0) {
    // just print the id
    return stringc << "id " << type->id;
  }
  else {
    // print id in a comment but go on to print the
    // type one level deeper
    return stringc << makeIdComment(type->id) << " "
                   << type->toCilString(depth);
  }
}


string AtomicType::toString(int depth) const
{
  return stringc << recurseCilString(this, depth+1)
                 << " /""* " << toCString() << " */";
}


// ---------------- CVFlags -------------
MAKE_ML_TAG(attribute, 0, AId)
MAKE_ML_TAG(attribute, 1, ACons)

MLValue cvToMLAttrs(CVFlags cv)
{
  // AId of string
  
  MLValue list = mlNil();
  if (cv & CV_CONST) {
    list = mlCons(mlTuple1(attribute_AId, mlString("const")), list);
  }
  if (cv & CV_VOLATILE) {
    list = mlCons(mlTuple1(attribute_AId, mlString("volatile")), list);
  }
  if (cv & CV_OWNER) {
    list = mlCons(mlTuple1(attribute_AId, mlString("owner")), list);
  }
  return list;
}



// ------------------ SimpleType -----------------
string SimpleType::toCString() const
{
  return simpleTypeName(type);
}


string SimpleType::toCilString(int) const
{
  return toCString();
}


int SimpleType::reprSize() const
{
  return simpleTypeReprSize(type);
}


#define MKTAG(n,t) MAKE_ML_TAG(typ, n, t)
MKTAG(0, TVoid)
MKTAG(1, TInt)
MKTAG(2, TBitfield)
MKTAG(3, TFloat)
MKTAG(4, Typedef)
MKTAG(5, TPtr)
MKTAG(6, TArray)
MKTAG(7, TStruct)
MKTAG(8, TUnion)
MKTAG(9, TEnum)
MKTAG(10, TFunc)
#undef MKTAG

#define MKTAG(n,t) MAKE_ML_TAG(ikind, n, t)
MKTAG(0, IChar)
MKTAG(1, ISChar)
MKTAG(2, IUChar)
MKTAG(3, IInt)
MKTAG(4, IUInt)
MKTAG(5, IShort)
MKTAG(6, IUShort)
MKTAG(7, ILong)
MKTAG(8, IULong)
MKTAG(9, ILongLong)
MKTAG(10, IULongLong)
#undef MKTAG

#define MKTAG(n,t) MAKE_ML_TAG(fkind, n, t)
MKTAG(0, FFloat)
MKTAG(1, FDouble)
MKTAG(2, FLongDouble)
#undef MKTAG


MLValue SimpleType::toMLValue(int, CVFlags cv) const
{
  // TVoid * attribute list
  // TInt of ikind * attribute list

  MLValue attrs = cvToMLAttrs(cv);

  #define TUP(t,i) return mlTuple2(typ_##t, mlTuple0(ikind_##i), attrs)
  switch (type) {
    default: xfailure("bad tag");
    case ST_CHAR:               TUP(TInt, IChar);
    case ST_UNSIGNED_CHAR:      TUP(TInt, IUChar);
    case ST_SIGNED_CHAR:        TUP(TInt, ISChar);
    case ST_BOOL:               TUP(TInt, IInt);        // ...
    case ST_INT:                TUP(TInt, IInt);
    case ST_UNSIGNED_INT:       TUP(TInt, IUInt);
    case ST_LONG_INT:           TUP(TInt, ILong);
    case ST_UNSIGNED_LONG_INT:  TUP(TInt, IULong);
    case ST_LONG_LONG:          TUP(TInt, ILongLong);
    case ST_UNSIGNED_LONG_LONG: TUP(TInt, IULongLong);
    case ST_SHORT_INT:          TUP(TInt, IShort);
    case ST_UNSIGNED_SHORT_INT: TUP(TInt, IUShort);
    case ST_WCHAR_T:            TUP(TInt, IShort);      // ...
  #undef TUP
  #define TUP(t,i) return mlTuple2(typ_##t, mlTuple0(fkind_##i), attrs)
    case ST_FLOAT:              TUP(TFloat, FFloat);
    case ST_DOUBLE:             TUP(TFloat, FDouble);
    case ST_LONG_DOUBLE:        TUP(TFloat, FLongDouble);
    case ST_VOID:               return mlTuple1(typ_TVoid, attrs);
  }
  #undef TUP
}


// ------------------ NamedAtomicType --------------------
NamedAtomicType::NamedAtomicType(char const *n)
  : name(n)
{}

NamedAtomicType::~NamedAtomicType()
{}


string NamedAtomicType::uniqueName() const
{
  // 'a' for atomic
  return stringc << "a" << id << "_" << name;
}


MLValue NamedAtomicType::toMLValue(int depth, CVFlags cv) const
{
  // we break the circularity at the entry to named atomics;
  // we'll emit typedefs for all of them beforehand
  xassert(depth >= 1);
  depth--;

  if (depth == 0) {
    // Typedef of string * int * typ ref * attribute list

    return mlTuple4(typ_Typedef,
                    mlString(uniqueName()),
                    mlInt(id),
                    mlRef(mlNil()),      // to be set in a post-process
                    cvToMLAttrs(cv));
  }
  else {
    // full info
    return toMLContentsValue(depth, cv);
  }
}


// ------------------ CompoundType -----------------
CompoundType::CompoundType(Keyword k, char const *n)
  : NamedAtomicType(n),
    keyword(k),
    env(NULL)
{}

CompoundType::~CompoundType()
{
  if (env) {
    delete env;
  }
}


void CompoundType::makeComplete(Env *parentEnv, TypeEnv *te,
                                VariableEnv *ve)
{
  xassert(!env);     // shouldn't have already made one yet
  env = new Env(parentEnv, te, ve);
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
  return stringc << keywordName(keyword) << " " << name;
}


// watch out for circular pointers (recursive types) here!
// (already bitten once ..)
string CompoundType::toStringWithFields() const
{
  stringBuilder sb;
  sb << toCString();
  if (isComplete()) {
    sb << " { " << env->toString() << "};";
  }
  else {
    sb << ";";
  }
  return sb;
}


string CompoundType::toCilString(int depth) const
{
  if (!isComplete()) {
    // this is a problem for my current type-printing
    // strategy, since I'm likely to print this even
    // when later I will get complete type info ..
    return "incomplete";
  }

  stringBuilder sb;
  sb << keywordName(keyword) << " " << name << " {\n";

  // iterate over fields
  // TODO2: this is not in the declared order ..
  StringSObjDict<Variable> &vars = env->getVariables();
  StringSObjDict<Variable>::Iter iter(vars);
  for (; !iter.isDone(); iter.next()) {
    Variable const *var = iter.value();
    sb << "  " << var->name << ": "
       << var->type->toString(depth-1) << ";\n";
  }

  sb << "}";
  return sb;
}


MLValue CompoundType::toMLContentsValue(int depth, CVFlags cv) const
{
  // build up field list
  MLValue fields = mlNil();
  StringSObjDict<Variable>::Iter iter(env->getVariables());
  for (; !iter.isDone(); iter.next()) {
    fields = mlCons(mlRecord4(
                      "fstruct", mlString(name),
                      "fname", mlString(iter.value()->name),
                      "typ", iter.value()->type->toMLValue(depth),
                      "fattr", mlRef(mlNil())
                    ), fields);
  }

  // TStruct of string * fieldinfo list * int * attribute list
  // TUnion of string * fieldinfo list * int * attribute list

  // assemble into a single tuple
  return mlTuple4(keyword==K_STRUCT? typ_TStruct : typ_TUnion,
                  mlString(name),
                  fields,
                  mlInt(id),
                  cvToMLAttrs(cv));
}


int CompoundType::reprSize() const
{
  int total = 0;
  StringSObjDict<Variable>::Iter iter(env->getVariables());
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


string EnumType::toCString() const
{
  return stringc << "enum " << name;
}


string EnumType::toCilString(int depth) const
{       
  // TODO2: get fields
  return toCString();
}


MLValue EnumType::toMLContentsValue(int, CVFlags cv) const
{
  // TEnum of string * (string * int) list * int * attribute list

  return mlTuple4(typ_TEnum,
                  mlString(name),
                  mlNil(),        // TODO2: get enum elements
                  mlInt(id),
                  cvToMLAttrs(cv));
}


int EnumType::reprSize() const
{
  // this is the usual choice
  return simpleTypeReprSize(ST_INT);
}


// ---------------- EnumValue --------------
EnumValue::EnumValue(char const *n, EnumType const *t, int v)
  : name(n), type(t), value(v)
{}

EnumValue::~EnumValue()
{}


// --------------- Type ---------------
ALLOC_STATS_DEFINE(Type)

Type::Type()
  : id(NULL_TYPEID)
{
  ALLOC_STATS_IN_CTOR
}

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
  return makeIdComment(id);
}


string Type::toCString() const
{
  return stringc << idComment() << leftString() << rightString();
}

string Type::toCString(char const *name) const
{
  return stringc << idComment()
                 << leftString() << " " << name << rightString();
}

string Type::rightString() const
{
  return "";
}


string Type::toString(int depth) const
{
  return stringc << recurseCilString(this, depth+1)
                 << " /""* " << toCString() << " */";
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

bool Type::isUnionType() const
{
  if (isCVAtomicType()) {
    AtomicType const *at = asCVAtomicTypeC().atomic;
    if (at->isCompoundType()) {
      return at->asCompoundTypeC().keyword == CompoundType::K_UNION;
    }
  }
  return false;
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


string CVAtomicType::atomicIdComment() const
{
  return makeIdComment(atomic->id);
}


string CVAtomicType::leftString() const
{
  return stringc << atomicIdComment()
                 << atomic->toCString() << cvToString(cv);
}


string CVAtomicType::toCilString(int depth) const
{
  return stringc << cvToString(cv) << " atomic "
                 << recurseCilString(atomic, depth);
}


MLValue CVAtomicType::toMLValue(int depth) const
{
  return atomic->toMLValue(depth, cv);
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


string PointerType::toCilString(int depth) const
{
  return stringc << cvToString(cv)
                 << (op==PO_POINTER? "ptrto " : "refto ")
                 << recurseCilString(atType, depth);
}


MLValue PointerType::toMLValue(int depth) const
{
  // TPtr of typ * attribute list

  return mlTuple2(typ_TPtr,
                  atType->toMLValue(depth),
                  cvToMLAttrs(cv));
}


int PointerType::reprSize() const
{
  // a typical value .. (architecture-dependent)
  return 4;
}


// -------------------- Parameter -----------------
Parameter::~Parameter()
{}


string Parameter::toString() const
{
  return type->toCString(name);
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


string FunctionType::toCilString(int depth) const
{
  stringBuilder sb;
  sb << "func " << cvToString(cv) << " ";
  if (acceptsVarargs) {
    sb << "varargs ";
  }
  sb << "(";

  int ct=0;
  FOREACH_OBJLIST(Parameter, params, iter) {
    if (++ct > 1) {
      sb << ", ";
    }
    sb << iter.data()->name << ": "
       << recurseCilString(iter.data()->type, depth);
  }

  sb << ") -> " << recurseCilString(retType, depth);

  return sb;
}


MLValue FunctionType::toMLValue(int depth) const
{
  // TFunc of typ * typ list * bool * attribute list

  // build up argument type list
  MLValue args = mlNil();
  FOREACH_OBJLIST(Parameter, params, iter) {
    args = mlCons(iter.data()->type->toMLValue(depth),
                  args);
  }

  return mlTuple4(typ_TFunc,
                  retType->toMLValue(depth),
                  args,
                  mlBool(acceptsVarargs),
                  cvToMLAttrs(cv));
}


int FunctionType::reprSize() const
{
  // thinking here about how this works when we're summing
  // the fields of a class with member functions ..
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


string ArrayType::toCilString(int depth) const
{
  stringBuilder sb;
  sb << "array [";
  if (hasSize) {
    sb << size;
  }
  sb << "] of " << recurseCilString(eltType, depth);
  return sb;
}


MLValue ArrayType::toMLValue(int depth) const
{
  // TArray of typ * exp option * attribute list

  // size
  MLValue mlSize;
  if (hasSize) {
    // since the array type is currently an arbitrary
    // expression, but I just store an int (because I
    // evaluate sizeof at parse time), construct an
    // expression now
    Owner<CilExpr> e; e = newIntLit(NULL /*extra*/, size);
    mlSize = mlSome(e->toMLString());
  }
  else {
    mlSize = mlNone();
  }

  return mlTuple3(typ_TArray,
                  eltType->toMLValue(depth),
                  mlSize,
                  mlNil());    // no attrs for arrays
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
