// cc_env.cc
// code for cc_env.h

#include "cc_env.h"      // this module
#include "trace.h"       // tracingSys
#include "ckheap.h"      // heapCheck
#include "strtable.h"    // StringTable


// ----------------------- Variable ---------------------------
Variable::Variable(StringRef n, DeclFlags d, Type const *t)
  : name(n),
    declFlags(d),
    type(t),
    declarator(NULL)
{
  xassert(type);        // (just a stab in the dark debugging effort)
}

Variable::~Variable()
{}


string Variable::toString() const
{
  // don't care about printing the declflags right now
  return type->toCString(name);
}


#if 0
MAKE_ML_TAG(storage, 0, NoStorage)
MAKE_ML_TAG(storage, 1, Static)
MAKE_ML_TAG(storage, 2, Register)
MAKE_ML_TAG(storage, 3, Extern)

extern MLValue unknownMLLoc();     // cil.cc

MLValue Variable::toMLValue() const
{
  //  type varinfo = {
  //      vid: int;		(* unique integer indentifier, one per decl *)
  //      vname: string;
  //      vglob: bool;	(* is this a global variable? *)
  //      vtype: typ;
  //      mutable vdecl: location;	(* where was this variable declared? *)
  //      mutable vattr: attribute list;
  //      mutable vstorage: storage;
  //  }

  return mlRecord7("vid", mlInt(id),
                   "vname", mlString(name),
                   "vglob", mlBool(isGlobal()),
                   "vtype", type->toMLValue(),
                   "vdecl", unknownMLLoc(),
                   "vattr", mlNil(),
                   "vstorage", mlStorage(declFlags));
}
#endif // 0


// --------------------- ScopedEnv ---------------------
ScopedEnv::ScopedEnv()
{}

ScopedEnv::~ScopedEnv()
{}


// --------------------------- Env ----------------------------
Env::Env(StringTable &table)
  : scopes(),
    typedefs(),
    compounds(),
    enums(),
    enumerators(),
    errors(0),
    compoundStack(),
    currentRetType(NULL),
    inPredicate(false),
    strTable(table)
{
  enterScope();

  #if 0
  declareVariable("__builtin_constant_p", DF_BUILTIN,
    makeFunctionType_1arg(
      getSimpleType(ST_INT),                          // return type
      CV_NONE,
      getSimpleType(ST_INT), "potentialConstant"),    // arg type
      true /*initialized*/);
  #endif // 0
}


Env::~Env()
{
  // explicitly free things, for easier debugging of dtor sequence
  scopes.deleteAll();
  typedefs.empty();
  compounds.empty();
  enums.empty();
  enumerators.empty();
}


void Env::enterScope()
{
  scopes.prepend(new ScopedEnv());
}

void Env::leaveScope()
{
  scopes.deleteAt(0);
}


// NOTE: the name lookup rules in this code have not been
// carefully checked against what the standard requires,
// so they are likely wrong; I intend to go through and
// make the correct at some point


// ---------------------------- variables ---------------------------
Variable *Env::
  addVariable(StringRef name, DeclFlags flags, Type const *type)
{
  // shouldn't be using this interface to add typedefs
  xassert(!(flags & DF_TYPEDEF));

  // special case for adding compound type members
  if (compoundStack.isNotEmpty()) {
    addCompoundField(compoundStack.first(), name, type);
    return NULL;     // ..hmm..
  }

  Variable *prev = getVariable(name, true /*inner*/);
  if (prev) {
    // if the old decl and new are the same array type,
    // except the old was missing a size, replace the
    // old with the new
    if (type->isArrayType() &&
        prev->type->isArrayType()) {
      ArrayType const *arr = &( type->asArrayTypeC() );
      ArrayType const *parr = &( prev->type->asArrayTypeC() );
      if (arr->eltType->equals(parr->eltType) &&
          arr->hasSize &&
          !parr->hasSize) {
        // replace old with new
        prev->type = type;
      }
    }

    // no way we allow it if the types don't match
    if (!type->equals(prev->type)) {
      errThrow(stringc
        << "conflicting declaration for `" << name
        << "'; previous type was `" << prev->type->toString()
        << "', this type is `" << type->toString() << "'");
    }

    // but it's ok if both were functions
    // and/or both were extern or static (TODO: what are the
    // real rules??); and, there can be at most one initializer
    if ( ( type->isFunctionType() ||
           ((flags & DF_EXTERN) && (prev->declFlags & DF_EXTERN)) ||
           ((flags & DF_STATIC) && (prev->declFlags & DF_STATIC))
         )
         //&& (!prev->isInitialized() || !initialized)
       ) {
      // ok
      return prev;
    }
    else {
      errThrow(stringc << "duplicate variable decl: " << name);
      return NULL;    // silence warning
    }
  }

  else /*not already mapped*/ {
    if (isGlobalEnv()) {
      flags = (DeclFlags)(flags | DF_GLOBAL);
    }

    Variable *ret = new Variable(name, flags, type);
    scopes.first()->variables.add(name, ret);
    return ret;
  }
}


Variable *Env::getVariable(StringRef name, bool innerOnly)
{
  MUTATE_EACH_OBJLIST(ScopedEnv, scopes, iter) {
    Variable *v;
    if (iter.data()->variables.query(name, v)) {
      return v;
    }
    
    if (innerOnly) {
      return NULL;    // don't look beyond the first
    }
  }
  
  return NULL;
}


// ------------------- typedef -------------------------
void Env::addTypedef(StringRef name, Type const *type)
{
  if (getTypedef(name)) {
    errThrow(stringc
      << "duplicate typedef for `" << name << "'");
  }
  typedefs.add(name, const_cast<Type*>(type));
}


Type const *Env::getTypedef(StringRef name)
{
  Type *t;
  if (typedefs.query(name, t)) {
    return t;
  }
  else {
    return NULL;
  }
}


// ----------------------- compounds -------------------
CompoundType *Env::addCompound(StringRef name, CompoundType::Keyword keyword)
{
  if (name && compounds.isMapped(name)) {
    errThrow(stringc << "compound already declared: " << name);
  }

  CompoundType *ret = new CompoundType(keyword, name);
  //grabAtomic(ret);
  if (name) {
    compounds.add(name, ret);
  }

  return ret;
}


void Env::addCompoundField(CompoundType *ct, StringRef name, Type const *type)
{
  if (ct->getNamedField(name)) {
    errThrow(stringc << "field already declared: " << name);
  }

  ct->addField(name, type);
}


CompoundType *Env::getCompound(StringRef name)
{
  CompoundType *e;
  if (name && compounds.query(name, e)) {
    return e;
  }
  else {
    return NULL;
  }
}


CompoundType *Env::getOrAddCompound(StringRef name, CompoundType::Keyword keyword)
{
  CompoundType *ret = getCompound(name);
  if (!ret) {
    return addCompound(name, keyword);
  }
  else {
    if (ret->keyword != keyword) {
      errThrow(stringc << "keyword mismatch for compound " << name);
    }
    return ret;
  }
}


// ---------------------- enums ---------------------
EnumType *Env::addEnum(StringRef name)
{
  if (name && enums.isMapped(name)) {
    errThrow(stringc << "enum already declared: " << name);
  }

  EnumType *ret = new EnumType(name);
  if (name) {
    enums.add(name, ret);
  }
  return ret;
}


EnumType *Env::getEnum(StringRef name)
{
  EnumType *ret;
  if (name && enums.query(name, ret)) {
    return ret;
  }
  else {
    return NULL;
  }
}


EnumType *Env::getOrAddEnum(StringRef name)
{
  EnumType *ret = getEnum(name);
  if (!ret) {
    return addEnum(name);
  }
  else {
    return ret;
  }
}


// ------------------ enumerators ---------------------
EnumType::Value *Env::addEnumerator(StringRef name, EnumType *et, int value)
{
  if (enumerators.isMapped(name)) {
    errThrow(stringc << "duplicate enumerator: " << name);
  }

  EnumType::Value *ret = et->addValue(name, value);
  enumerators.add(name, ret);
  return ret;
}


EnumType::Value *Env::getEnumerator(StringRef name)
{
  EnumType::Value *ret;
  if (enumerators.query(name, ret)) {
    return ret;
  }
  else {
    return NULL;
  }
}


// -------------------- type construction ------------------
CVAtomicType *Env::makeType(AtomicType const *atomic)
{
  return makeCVType(atomic, CV_NONE);
}


CVAtomicType *Env::makeCVType(AtomicType const *atomic, CVFlags cv)
{
  CVAtomicType *ret = new CVAtomicType(atomic, cv);
  grab(ret);
  return ret;
}


Type const *Env::applyCVToType(CVFlags cv, Type const *baseType)
{
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
      CVAtomicType const &atomic = baseType->asCVAtomicTypeC();
      if ((atomic.cv | cv) == atomic.cv) {
        // the given type already contains 'cv' as a subset,
        // so no modification is necessary
        return baseType;
      }
      else {
        // we have to add another CV, so that means creating
        // a new CVAtomicType with the same AtomicType as 'baseType'
        CVAtomicType *ret = new CVAtomicType(atomic);
        grab(ret);

        // but with the new flags added
        ret->cv = (CVFlags)(ret->cv | cv);

        return ret;
      }
      break;
    }

    case Type::T_POINTER: {
      // logic here is nearly identical to the T_ATOMIC case
      PointerType const &ptr = baseType->asPointerTypeC();
      if (ptr.op == PO_REFERENCE) {
        return NULL;     // can't apply CV to references
      }
      if ((ptr.cv | cv) == ptr.cv) {
        return baseType;
      }
      else {
        PointerType *ret = new PointerType(ptr);
        grab(ret);
        ret->cv = (CVFlags)(ret->cv | cv);
        return ret;
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


ArrayType const *Env::setArraySize(ArrayType const *type, int size)
{                                      
  ArrayType *ret = new ArrayType(type->eltType, size);
  grab(ret);
  return ret;
}


PointerType *Env::makePtrOperType(PtrOper op, CVFlags cv, Type const *type)
{
  PointerType *ret = new PointerType(op, cv, type);
  grab(ret);
  return ret;
}


FunctionType *Env::makeFunctionType(Type const *retType/*, CVFlags cv*/)
{
  FunctionType *ret = new FunctionType(retType/*, cv*/);
  grab(ret);
  return ret;
}


#if 0
FunctionType *Env::makeFunctionType_1arg(
  Type const *retType, CVFlags cv,
  Type const *arg1Type, char const *arg1Name)
{
  FunctionType *ret = makeFunctionType(retType/*, cv*/);
  ret->addParam(new Parameter(arg1Type, arg1Name));
  return ret;
}    
#endif // 0


ArrayType *Env::makeArrayType(Type const *eltType, int size)
{
  ArrayType *ret = new ArrayType(eltType, size);
  grab(ret);
  return ret;
}

ArrayType *Env::makeArrayType(Type const *eltType)
{
  ArrayType *ret = new ArrayType(eltType);
  grab(ret);
  return ret;
}


void Env::checkCoercible(Type const *src, Type const *dest)
{
  // just say yes
}

Type const *Env::promoteTypes(BinaryOp op, Type const *t1, Type const *t2)
{
  // yes yes yes
  return t1;
}


// --------------------- error/warning reporting ------------------
void Env::err(char const *str)
{
  cout << "error: " << str << endl;
  errors++;
}


void Env::errThrow(char const *str)
{
  err(str);
  THROW(XError(str));
}


void Env::errIf(bool condition, char const *str)
{
  if (condition) {
    errThrow(str);
  }
}


// ---------------------- debugging ---------------------
string Env::toString() const
{
  stringBuilder sb;

  // for now, just the variables
  FOREACH_OBJLIST(ScopedEnv, scopes, sc) {
    StringObjDict<Variable>::Iter iter(sc.data()->variables);
    for (; !iter.isDone(); iter.next()) {
      sb << iter.value()->toString() << " ";
    }
  }

  return sb;
}


void Env::selfCheck() const
{}


